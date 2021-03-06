// kmfolder.cpp
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <qfileinf.h>

#include "kmglobal.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmfolderdir.h"

#include <kapp.h>
#include <mimelib/mimepp.h>
#include <qregexp.h>

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_FCNTL_H && !HAVE_FLOCK
#include <fcntl.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#ifndef isblank
#  define isblank(x) ((x)==' '||(x)=='\t')
#endif

#define MAX_LINE 4096
#define INIT_MSGS 8

// Current version of the table of contents (index) files
#define INDEX_VERSION 1202

// Regular expression to find the line that seperates messages in a mail
// folder:
#define MSG_SEPERATOR_START "From "
#define MSG_SEPERATOR_REGEX "^From .*..:...*$"
static short msgSepLen = strlen(MSG_SEPERATOR_START);


static int _rename(const char* oldname, const char* newname)
{
  return rename(oldname, newname);
}


//-----------------------------------------------------------------------------
KMFolder :: KMFolder(KMFolderDir* aParent, const char* aName) : 
  KMFolderInherited(aParent, aName), mMsgList(INIT_MSGS)
{
  //-- in case that the compiler has problems with the static version above:
  //msgSepLen = strlen(MSG_SEPERATOR_START);

  initMetaObject();

  mStream         = NULL;
  mIndexStream    = NULL;
  mOpenCount      = 0;
  mQuiet          = 0;
  mHeaderOffset   = 0;
  mAutoCreateIndex= TRUE;
  mFilesLocked    = FALSE;
  mIsSystemFolder = FALSE;
  mType           = "plain";
  mAcctList       = NULL;
  mDirty          = FALSE;
  mWhoField       = 0;
  unreadMsgs      = -1;
  needsCompact    = false;
}


//-----------------------------------------------------------------------------
KMFolder :: ~KMFolder()
{
  if (mOpenCount>0) close(TRUE);

  //if (mAcctList) delete mAcctList;
  /* Well, this is a problem. If I add the above line then kmfolder depends
   * on kmaccount and is then not that portable. Hmm.
   */
}


//-----------------------------------------------------------------------------
const QString KMFolder::location(void) const
{
  QString sLocation;

  sLocation = path().copy();
  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += name();

  return sLocation;
}


//-----------------------------------------------------------------------------
const QString KMFolder::indexLocation(void) const
{
  QString sLocation;

  sLocation = path().copy();
  if (!sLocation.isEmpty()) sLocation += '/';
  sLocation += '.';
  sLocation += name();
  sLocation += ".index";

  return sLocation;
}


//-----------------------------------------------------------------------------
int KMFolder::open(void)
{
  int rc = 0;

  mOpenCount++;
  if (mOpenCount > 1) return 0;  // already open

  assert(name() != "");

  mFilesLocked = FALSE;
  mStream = fopen(location(), "r+"); // messages file
  if (!mStream) 
  {
    debug("Cannot open folder `%s': %s", (const char*)location(), 
	  strerror(errno));
    mOpenCount = 0;
    return errno;
  }

  if (!path().isEmpty())
  {
    if (isIndexOutdated()) // test if contents file has changed
    {
      QString str;
      mIndexStream = NULL;
      str.sprintf(i18n("Folder `%s' changed. Recreating index."), 
		  (const char*)name());
      emit statusMsg(str);
    }
    else mIndexStream = fopen(indexLocation(), "r+"); // index file

    if (!mIndexStream) rc = createIndexFromContents();
    else readIndex();
  }
  else
  {
    //debug("No path specified for folder `" + name() +
    //      "' -- Turning autoCreateIndex off");
    mAutoCreateIndex = FALSE;
    rc = createIndexFromContents();
  }

  if (!rc) lock();
  mQuiet = 0;

  return rc;
}


//-----------------------------------------------------------------------------
int KMFolder::create(void)
{
  int rc;
  int old_umask;

  assert(name() != "");
  assert(mOpenCount == 0);

  if (access(location(), F_OK) == 0) return EEXIST;
  
  old_umask = umask(077);
  mStream = fopen(location(), "w+"); //sven; open RW
  umask(old_umask);

  if (!mStream) return errno;

  if (!path().isEmpty())
  {
    old_umask = umask(077);
    mIndexStream = fopen(indexLocation(), "w+"); //sven; open RW
    umask(old_umask);

    if (!mIndexStream) return errno;
  }
  else
  {
    //debug("Folder `" + name() +
    //      "' has no path specified -- turning autoCreateIndex off");
    mAutoCreateIndex = FALSE;
  }

  mOpenCount++;
  mQuiet = 0;

  rc = writeIndex();
  if (!rc) lock();
  return rc;
}


//-----------------------------------------------------------------------------
void KMFolder::close(bool aForced)
{
  if (mOpenCount <= 0 || !mStream) return;
  if (mOpenCount > 0) mOpenCount--;
  if (mOpenCount > 0 && !aForced) return;
  if (mAutoCreateIndex) 
  {
    if (mDirty) writeIndex();
    else sync();
  }

  unlock();
  mMsgList.clear(TRUE);

  if (mStream) fclose(mStream);
  if (mIndexStream) fclose(mIndexStream);

  mOpenCount   = 0;
  mStream      = NULL;
  mIndexStream = NULL;
  mFilesLocked = FALSE;

  mMsgList.reset(INIT_MSGS);
}


//-----------------------------------------------------------------------------
int KMFolder::lock(void)
{
  int rc;
#if !HAVE_FLOCK
  struct flock fl;
  fl.l_type=F_WRLCK;
  fl.l_whence=0;
  fl.l_start=0;
  fl.l_len=0;
#endif

  assert(mStream != NULL);
  mFilesLocked = FALSE;

#if HAVE_FLOCK
  rc = flock(fileno(mStream), LOCK_NB|LOCK_EX);
#else
  rc = fcntl(fileno(mStream), F_SETLK, &fl);
#endif

  if (rc < 0)
  {
    debug("Cannot lock folder `%s': %s (%d)", (const char*)location(),
	  strerror(errno), errno);
    return errno;
  }

  if (mIndexStream)
  {
#if HAVE_FLOCK
    rc = flock(fileno(mIndexStream), LOCK_UN);
#else
    rc = fcntl(fileno(mIndexStream), F_SETLK, &fl);
#endif

    if (rc < 0) 
    {
      debug("Cannot lock index of folder `%s': %s", (const char*)location(),
	    strerror(errno));
      rc = errno;
#if HAVE_FLOCK
      rc = flock(fileno(mIndexStream), LOCK_UN);
#else
      fl.l_type = F_UNLCK;
      rc = fcntl(fileno(mIndexStream), F_SETLK, &fl);
#endif
      return rc;
    }
  }

  mFilesLocked = TRUE;
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::unlock(void)
{
  int rc;
#if !HAVE_FLOCK
  struct flock fl;
  fl.l_type=F_UNLCK;
  fl.l_whence=0;
  fl.l_start=0;
  fl.l_len=0;
#endif

  assert(mStream != NULL);
  mFilesLocked = FALSE;

#if HAVE_FLOCK
  if (mIndexStream) flock(fileno(mIndexStream), LOCK_UN);
  rc = flock(fileno(mStream), LOCK_UN);
#else
  if (mIndexStream) fcntl(fileno(mIndexStream), F_SETLK, &fl);
  rc = fcntl(fileno(mStream), F_SETLK, F_UNLCK);
#endif

  return errno;
}


//-----------------------------------------------------------------------------
bool KMFolder::isIndexOutdated(void)
{
  QFileInfo contInfo(location());
  QFileInfo indInfo(indexLocation());

  if (!contInfo.exists()) return FALSE;
  if (!indInfo.exists()) return TRUE;

  return (contInfo.lastModified() > indInfo.lastModified());
}


//-----------------------------------------------------------------------------
int KMFolder::createIndexFromContents(void)
{
  char line[MAX_LINE];
  char status[8], xstatus[8];
  QString subjStr, dateStr, fromStr, toStr, xmarkStr, *lastStr=NULL;
  QString whoFieldName;
  unsigned long offs, size, pos;
  bool atEof = FALSE;
  bool inHeader = TRUE;
  KMMsgInfo* mi;
  QString msgStr(256);
  QRegExp regexp(MSG_SEPERATOR_REGEX);
  int i, num, numStatus, whoFieldLen;
  short needStatus;

  assert(mStream != NULL);
  rewind(mStream);

  mMsgList.clear();

  num     = -1;
  numStatus= 11;
  offs    = 0;
  size    = 0;
  dateStr = "";
  fromStr = "";
  toStr = "";
  subjStr = "";
  *status = '\0';
  *xstatus = '\0';
  xmarkStr = "";
  needStatus = 3;
  whoFieldName = QString(whoField()) + ':'; //unused (sven)
  whoFieldLen = whoFieldName.length();      //unused (sven)

  //debug("***whoField: %s (%d)",
  //      (const char*)whoFieldName, whoFieldLen);

  while (!atEof)
  {
    pos = ftell(mStream);
    if (!fgets(line, MAX_LINE, mStream)) atEof = TRUE;

    if (atEof ||
	(strncmp(line,MSG_SEPERATOR_START, msgSepLen)==0 && 
	 regexp.match(line) >= 0))
    {
      size = pos - offs;
      pos = ftell(mStream);

      if (num >= 0)
      {
	if (numStatus <= 0)
	{
	  msgStr.sprintf(i18n("Creating index file: %d messages done"), num);
	  emit statusMsg(msgStr);
	  numStatus = 10;
	}

	if (size > 0)
	{
	  mi = new KMMsgInfo(this);
	  mi->init(subjStr, fromStr, toStr, 0, KMMsgStatusNew, xmarkStr, offs, size);
	  mi->setStatus(status,xstatus);
	  mi->setDate(dateStr);
	  mi->setDirty(FALSE);
	  mMsgList.append(mi);

	  *status = '\0';
	  *xstatus = '\0';
	  needStatus = 3;
	  xmarkStr = "";
	  dateStr = "";
	  fromStr = "";
	  subjStr = "";
	}
	else num--,numStatus++;
      }

      offs = ftell(mStream);
      num++;
      numStatus--;
      inHeader = TRUE;
      continue;
    }
    // Is this a long header line?
    if (inHeader && (line[0]=='\t' || line[0]==' '))
    {
      i = 0;
      while (line [i]=='\t' || line [i]==' ') i++;
      if (line [i] < ' ' && line [i]>0) inHeader = FALSE;
      else if (lastStr) *lastStr += line + i;
    } 
    else lastStr = NULL;

    if (inHeader && (line [0]=='\n' || line [0]=='\r'))
      inHeader = FALSE;
    if (!inHeader) continue;

    if ((needStatus & 1) && strncasecmp(line, "Status:", 7) == 0 && 
	isblank(line[7])) 
    {
      for(i=0; i<4 && line[i+8] > ' '; i++)
	status[i] = line[i+8];
      status[i] = '\0';
      needStatus &= ~1;
    }
    else if ((needStatus & 2) && strncasecmp(line, "X-Status:", 9)==0 &&
	     isblank(line[9]))
    {
      for(i=0; i<4 && line[i+10] > ' '; i++)
	xstatus[i] = line[i+10];
      xstatus[i] = '\0';
      needStatus &= ~2;
    }
    else if (strncasecmp(line,"X-KMail-Mark:",13)==0 && isblank(line[13]))
      xmarkStr = QString(line+14).copy();
    else if (strncasecmp(line,"Date:",5)==0 && isblank(line[5]))
    {
      dateStr = QString(line+6).copy();
      lastStr = &dateStr;
    }
    else if (strncasecmp(line,"From:", 5)==0 &&
	     isblank(line[5]))
    {
      fromStr = QString(line+6).copy();
      lastStr = &fromStr;
    }
    else if (strncasecmp(line,"To:", 3)==0 &&
	     isblank(line[3]))
    {
      toStr = QString(line+4).copy();
      lastStr = &toStr;
    }
    else if (strncasecmp(line,"Subject:",8)==0 && isblank(line[8]))
    {
      subjStr = QString(line+9).copy();
      lastStr = &subjStr;
    }
  }

  if (mAutoCreateIndex)
  {
    emit statusMsg(i18n("Writing index file"));
    writeIndex();
  }
  else mHeaderOffset = 0;

  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::writeIndex(void)
{
  KMMsgBase* msgBase;
  int old_umask;
  int i=0;

  if (mIndexStream) fclose(mIndexStream);
  old_umask = umask(077);
  mIndexStream = fopen(indexLocation(), "w");
  umask(old_umask);
  if (!mIndexStream) return errno;

  fprintf(mIndexStream, "# KMail-Index V%d\n", INDEX_VERSION);

  mHeaderOffset = ftell(mIndexStream);
  for (i=0; i<mMsgList.high(); i++)
  {
    if (!(msgBase = mMsgList[i])) continue;
    fprintf(mIndexStream, "%s\n", (const char*)msgBase->asIndexString());
  }
  fflush(mIndexStream);

  mDirty = FALSE;
  return 0;
}


//-----------------------------------------------------------------------------
void KMFolder::setAutoCreateIndex(bool autoIndex)
{
  mAutoCreateIndex = autoIndex;
}


//-----------------------------------------------------------------------------
bool KMFolder::readIndexHeader(void)
{
  int indexVersion;

  assert(mIndexStream != NULL);

  fscanf(mIndexStream, "# KMail-Index V%d\n", &indexVersion);
  if (indexVersion < INDEX_VERSION)
  {
    debug("Index file %s is out of date. Re-creating it.", 
	  (const char*)indexLocation());
    createIndexFromContents();
    return FALSE;
  }

  return TRUE;
}


//-----------------------------------------------------------------------------
void KMFolder::readIndex(void)
{
  char line[MAX_LINE];
  KMMsgInfo* mi;

  assert(mIndexStream != NULL);
  rewind(mIndexStream);

  mMsgList.clear();
  if (!readIndexHeader()) return;

  mDirty = FALSE;
  mHeaderOffset = ftell(mIndexStream);

  mMsgList.clear();
  while (!feof(mIndexStream))
  {
    fgets(line, MAX_LINE, mIndexStream);
    if (feof(mIndexStream)) break;

    mi = new KMMsgInfo(this);
    mi->fromIndexString(line);
    if (mi->status() == KMMsgStatusDeleted) 
    {
      delete mi;  // skip messages that are marked as deleted
      mDirty = TRUE;
      needsCompact = true;  //We have deleted messages - needs to be compacted
      continue;
    }
#ifdef OBSOLETE
    else if (mi->status() == KMMsgStatusNew)
    {
      mi->setStatus(KMMsgStatusUnread);
      mi->setDirty(FALSE);
    }
#endif
    mMsgList.append(mi);
  }
}


//-----------------------------------------------------------------------------
void KMFolder::markNewAsUnread(void)
{
  KMMsgBase* msgBase;
  int i;

  for (i=0; i<mMsgList.high(); i++)
  {
    if (!(msgBase = mMsgList[i])) continue;
    if (msgBase->status() == KMMsgStatusNew)
    {
      msgBase->setStatus(KMMsgStatusUnread);
      msgBase->setDirty(TRUE);
    }
  }
}


//-----------------------------------------------------------------------------
void KMFolder::quiet(bool beQuiet)
{
  if (beQuiet) mQuiet++;
  else 
  {
    mQuiet--;
    if (mQuiet <= 0)
    {
      mQuiet = 0;
      emit changed();
    }
  }
}


//-----------------------------------------------------------------------------
void KMFolder::removeMsg(KMMsgBasePtr aMsg)
{
  removeMsg(find(aMsg));
}


//-----------------------------------------------------------------------------
void KMFolder::removeMsg(int idx)
{
  //assert(idx>=0);
  if(idx < 0)
    {
      debug("KMFolder::removeMsg() : idx < 0\n");
      return;
    }
  mMsgList.take(idx);
  mDirty = TRUE;
  if (!mQuiet) emit msgRemoved(idx);
}


//-----------------------------------------------------------------------------
KMMessage* KMFolder::take(int idx)
{
  KMMsgBase* mb;
  KMMessage* msg;

  assert(mStream!=NULL);
  assert(idx>=0 && idx<=mMsgList.high());

  mb = mMsgList[idx];
  if (!mb) return NULL;
  if (!mb->isMessage()) readMsg(idx);

  msg = (KMMessage*)mMsgList.take(idx);
  if (msg->status()==KMMsgStatusUnread ||
	  msg->status()==KMMsgStatusNew) {
	--unreadMsgs;
	emit numUnreadMsgsChanged( this );
  }
  msg->setParent(NULL);
  mDirty = TRUE;
  needsCompact=true; // message is taken from here - needs to be compacted
  if (!mQuiet) emit msgRemoved(idx);

  return msg;
}


//-----------------------------------------------------------------------------
KMMessage* KMFolder::getMsg(int idx)
{
  KMMsgBase* mb;

  // assert(idx>=0 && idx<=mMsgList.high());
  if(!(idx >= 0 && idx <= mMsgList.high()))
    return 0L;

  mb = mMsgList[idx];
  if (!mb) return NULL;

  if (mb->isMessage()) return ((KMMessage*)mb);
  return readMsg(idx);
}


//-----------------------------------------------------------------------------
KMMessage* KMFolder::readMsg(int idx)
{
  KMMessage* msg;
  unsigned long msgSize;
  QString msgText;
  KMMsgInfo* mi = (KMMsgInfo*)mMsgList[idx];

  assert(mi!=NULL && !mi->isMessage());
  assert(mStream != NULL);

  msgSize = mi->msgSize();
  msgText.resize(msgSize+2);

  fseek(mStream, mi->folderOffset(), SEEK_SET);
  fread(msgText.data(), msgSize, 1, mStream);
  msgText[msgSize] = '\0';

  msg = new KMMessage(*mi);
  msg->fromString(msgText);

  mMsgList.set(idx,msg);

  return msg;
}


//-----------------------------------------------------------------------------
int KMFolder::moveMsg(KMMessage* aMsg, int* aIndex_ret)
{
  KMFolder* msgParent;
  int rc;

  assert(aMsg != NULL);
  msgParent = aMsg->parent();

  if (msgParent)
  {
    msgParent->open();
    aMsg = msgParent->take(msgParent->find(aMsg));
    msgParent->close();
  }

  open();
  rc = addMsg(aMsg, aIndex_ret);
  close();

  // debug("KMFolder::moveMsg() rc=%i",rc);
  return rc;
}


//-----------------------------------------------------------------------------
int KMFolder::addMsg(KMMessage* aMsg, int* aIndex_ret)
{
  long offs, size, len;
  bool opened = FALSE;
  QString msgText;
  char endStr[3];
  int idx, rc;
  KMFolder* msgParent;
  bool editing = false;

  if (!mStream)
  {
    opened = TRUE;
    rc = open();
    debug("addMsg-open: %d", rc);
    if (rc) return rc;
  }

  // take message out of the folder it is currently in, if any
  msgParent = aMsg->parent();
  if (msgParent)
  {
    if (msgParent==this)
    {
      if (name() == "outbox") //special case for Edit message.
      {
        // debug ("Editing message in outbox");
        editing = true;
      }
      else
        return 0;
    }
    idx = msgParent->find(aMsg);
    if (idx >= 0) msgParent->take(idx);
  }

  aMsg->setStatusFields();
  msgText = aMsg->asString();
  len = msgText.length();

  if (aMsg->status()==KMMsgStatusUnread ||
	  aMsg->status()==KMMsgStatusNew) {
	++unreadMsgs;
	emit numUnreadMsgsChanged( this );
  }

  assert(mStream != NULL);
  if (len <= 0)
  {
    debug("Message added to folder `%s' contains no data. Ignoring it.",
	  (const char*)name());
    if (opened) close();
    return 0;
  }

  // write message to folder file
  fseek(mStream, -2, SEEK_END);
  fread(endStr, 1, 2, mStream); // ensure separating empty line
  if (ftell(mStream) > 0 && endStr[0]!='\n') 
  {
    if (endStr[1]!='\n')
    {
      //printf ("****endStr[1]=%c\n", endStr[1]);
      fwrite("\n\n", 1, 2, mStream);
    }
    else fwrite("\n", 1, 1, mStream);
  }
  fseek(mStream,0,SEEK_END); // this is needed on solaris and others
  fwrite("From aaa@aaa Mon Jan 01 00:00:00 1997\n", 38, 1, mStream);
  offs = ftell(mStream);
  fwrite(msgText, len, 1, mStream);
  if (msgText[len-1]!='\n') fwrite("\n\n", 1, 2, mStream);
  fflush(mStream);
  size = ftell(mStream) - offs;

  // store information about the position in the folder file in the message
  aMsg->setParent(this);
  aMsg->setFolderOffset(offs);
  aMsg->setMsgSize(size);
  idx = mMsgList.append(aMsg);

  // write index entry if desired
  if (mAutoCreateIndex)
  {
    assert(mIndexStream != NULL);
    fseek(mIndexStream, 0, SEEK_END);
    fprintf(mIndexStream, "%s\n", (const char*)aMsg->asIndexString()); 
    fflush(mIndexStream);
  }

  // some "paper work"
  if (aIndex_ret) *aIndex_ret = idx;
  if (!mQuiet) emit msgAdded(idx);
  if (opened) close();

  return 0;
} 


//-----------------------------------------------------------------------------
int KMFolder::rename(const QString aName)
{
  QString oldLoc, oldIndexLoc, newLoc, newIndexLoc, oldName;
  int rc=0, openCount=mOpenCount;

  assert(!aName.isEmpty());

  oldLoc = location().copy();
  oldIndexLoc = indexLocation().copy();

  close(TRUE);

  oldName = name();
  setName(aName);

  newLoc = location();
  newIndexLoc = indexLocation();

  if (_rename(oldLoc, newLoc)) 
  {
    setName(oldName);
    rc = errno;
  }
  else if (!oldIndexLoc.isEmpty()) _rename(oldIndexLoc, newIndexLoc);

  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }

  return rc;
}


//-----------------------------------------------------------------------------
int KMFolder::remove(void)
{
  int rc;

  assert(name() != "");

  close(TRUE);
  unlink(indexLocation());
  rc = unlink(location());
  if (rc) return rc;

  mMsgList.reset(INIT_MSGS);
  needsCompact = false; //we are dead - no need to compact us
  return 0;
}
	

//-----------------------------------------------------------------------------
int KMFolder::expunge(void)
{
  int openCount = mOpenCount;

  assert(name() != "");

  close(TRUE);

  if (mAutoCreateIndex) truncate(indexLocation(), mHeaderOffset);
  else unlink(indexLocation());

  if (truncate(location(), 0)) return errno;
  mDirty = FALSE;

  mMsgList.reset(INIT_MSGS);
  needsCompact = false; //we're cleared and truncated no need to compact
  
  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }

  unreadMsgs = 0;
  emit numUnreadMsgsChanged( this );
  if (!mQuiet) emit changed();
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::compact(void)
{
  KMFolder* tempFolder;
  KMMessage* msg;
  QString tempName, msgStr;
  int openCount = mOpenCount;
  int num, numStatus;


  if (!needsCompact)
  {
    //debug ("Not compacting %s; it's clean", name().data());
    return 0;
  }
  
  debug ("Compacting %s ", name().data());
  
  tempName = "." + name();
  tempName.detach();
  tempName += ".compacted";
  unlink(tempName);
  //tempFolder = parent()->createFolder(tempName); //sven: shouldn't be in list
  tempFolder = new KMFolder(parent(), tempName);   //sven: we create it
  if(tempFolder->create()) {
    debug("KMFolder::compact() Creating tempFolder failed!\n");
    delete tempFolder;                             //sven: and we delete it
    return 0;
  }

  quiet(TRUE);
  tempFolder->open();
  open();

  for(num=1,numStatus=9; count() > 0; num++, numStatus--)
  {
    if (numStatus <= 0)
    {
      msgStr.sprintf(i18n("Compacting folder: %d messages done"), num);
      emit statusMsg(msgStr);
      numStatus = 10;
    }

    msg = getMsg(0);
    if(msg)
      tempFolder->moveMsg(msg);
  }
  tempName = tempFolder->location();
  tempFolder->close(TRUE);
  close(TRUE);
  mMsgList.clear(TRUE);

  _rename(tempName, location());
  _rename(tempFolder->indexLocation(), indexLocation());

  // Now really free all memory
  delete tempFolder;                //sven: we delete it, not the manager

  if (openCount > 0)
  {
    open();
    mOpenCount = openCount;
  }
  quiet(FALSE);

  if (!mQuiet) emit changed();
  needsCompact = false;             // We are clean now
  return 0;
}


//-----------------------------------------------------------------------------
int KMFolder::sync(void)
{
  KMMsgBasePtr mb;
  unsigned long offset = mHeaderOffset;
  int i, rc, recSize = KMMsgBase::indexStringLength()+1;
  int high = mMsgList.high();

  if (!mIndexStream) return 0;

  for (rc=0,i=0; i<high; i++)
  {
    mb = mMsgList[i];
    if (mb->dirty())
    {
      fseek(mIndexStream, offset, SEEK_SET);
      fprintf(mIndexStream, "%s\n", (const char*)mb->asIndexString());
      rc = errno;
      if (rc) break;
      mb->setDirty(FALSE);
    }
    offset += recSize;
  }
  fflush(mIndexStream);
  mDirty = FALSE;

  return rc;
}


//-----------------------------------------------------------------------------
void KMFolder::sort(KMMsgList::SortField aField, bool aDesc)
{
  //debug("KMFolder::sort()");
  mMsgList.sort(aField, aDesc);
  //debug("KMFolder::sort() after");
  if (!mQuiet) emit changed();
  mDirty = TRUE;
}


//-----------------------------------------------------------------------------
const char* KMFolder::type(void) const
{
  if (mAcctList) return "In";
  return KMFolderInherited::type();
}


//-----------------------------------------------------------------------------
const QString KMFolder::label(void) const
{
  if (mIsSystemFolder && !mLabel.isEmpty()) return mLabel;
  if (unreadMsgs > 0) {
    QString num;
    num.setNum(unreadMsgs);
	return (name() + " (" + num + ")");
  }
  else
  return name();
}


//-----------------------------------------------------------------------------
long KMFolder::countUnread(void)
{
  register int  i;

  if (unreadMsgs != -1)
    return unreadMsgs;

  for (i=0, unreadMsgs=0; i<mMsgList.high(); i++)
  {
    if (mMsgList[i]->status()==KMMsgStatusUnread ||
	mMsgList[i]->status()==KMMsgStatusNew)
      unreadMsgs++;
  }

  return unreadMsgs;
}

//-----------------------------------------------------------------------------
void KMFolder::msgStatusChanged(const KMMsgStatus oldStatus,
	const KMMsgStatus newStatus)
{
  int oldUnread = 0;
  int newUnread = 0;
  if (oldStatus==KMMsgStatusUnread || oldStatus==KMMsgStatusNew)
    oldUnread = 1;
  if (newStatus==KMMsgStatusUnread || newStatus==KMMsgStatusNew)
    newUnread = 1;
  int deltaUnread = newUnread - oldUnread;
  if (deltaUnread != 0) {
	unreadMsgs += deltaUnread;
	emit numUnreadMsgsChanged( this );
  }
}

//-----------------------------------------------------------------------------
void KMFolder::headerOfMsgChanged(const KMMsgBase* aMsg)
{
  int idx = mMsgList.find((KMMsgBasePtr)aMsg);
  if (idx >= 0 && !mQuiet) emit msgHeaderChanged(idx);
}


//-----------------------------------------------------------------------------
const char* KMFolder::whoField(void) const
{
  return (mWhoField.isEmpty() ? "From" : (const char*)mWhoField);
}


//-----------------------------------------------------------------------------
void KMFolder::setWhoField(const QString& aWhoField)
{
  mWhoField = aWhoField.copy();
}


//-----------------------------------------------------------------------------
#include "kmfolder.moc"
