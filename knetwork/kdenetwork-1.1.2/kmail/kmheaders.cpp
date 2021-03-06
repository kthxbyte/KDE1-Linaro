// kmheaders.cpp


#include <qstrlist.h>
#include "kmcomposewin.h"
#include <kfiledialog.h>
#include <drag.h>
#include <qpalette.h>
#include <qcolor.h>

#include <kapp.h>
#include <kiconloader.h>

#include "kmfolder.h"
#include "kmheaders.h"
#include "kmmessage.h"
#include "kbusyptr.h"
#include "kmdragdata.h"
#include "kmglobal.h"
#include "kmmainwin.h"
#include "kfileio.h"
#include "kmfiltermgr.h"
#include "kfontutils.h"

#include "kmreaderwin.h"

//-----------------------------------------------------------------------------
KMHeaders::KMHeaders(KMMainWin *aOwner, QWidget *parent,
		     const char *name) :
  KMHeadersInherited(parent, name, 4)
{
  KIconLoader* loader = app->getIconLoader();
  static QPixmap pixNew, pixUns, pixDel, pixOld, pixRep, pixSent, pixQueued,
                 pixFwd;

  mOwner  = aOwner;
  mFolder = NULL;
  getMsgIndex = -1;
  mSortCol = KMMsgList::sfDate;
  mSortDescending = FALSE;
  mTopItem = 0;

  setColumn(0, i18n("F"), 18, KTabListBox::PixmapColumn);
  setColumn(1, i18n("Sender"), 200, KTabListBox::TextColumn);
  setColumn(2, i18n("Subject"), 270, KTabListBox::TextColumn);
  setColumn(3, i18n("Date"), 300, KTabListBox::TextColumn);
  readConfig();

  pixNew   = loader->loadIcon("kmmsgnew.xpm");
  pixUns   = loader->loadIcon("kmmsgunseen.xpm");
  pixDel   = loader->loadIcon("kmmsgdel.xpm");
  pixOld   = loader->loadIcon("kmmsgold.xpm");
  pixRep   = loader->loadIcon("kmmsgreplied.xpm");
  pixQueued= loader->loadIcon("kmmsgqueued.xpm");
  pixSent  = loader->loadIcon("kmmsgsent.xpm");
  pixFwd   = loader->loadIcon("kmmsgforwarded.xpm");

  dict().insert(KMMsgBase::statusToStr(KMMsgStatusNew), &pixNew);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusUnread), &pixUns);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusRead), &pixOld);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusDeleted), &pixDel);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusOld), &pixOld);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusReplied), &pixRep);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusForwarded), &pixFwd);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusQueued), &pixQueued);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusSent), &pixSent);

  connect(this,SIGNAL(selected(int,int)),
	  this,SLOT(selectMessage(int,int)));
  connect(this,SIGNAL(highlighted(int,int)),
          this,SLOT(highlightMessage(int,int)));
  connect(this,SIGNAL(popupMenu(int,int)),
          this,SLOT(slotRMB(int,int)));
  connect(this,SIGNAL(headerClicked(int)),
	  this,SLOT(headerClicked(int)));

  readConfig();
}


//-----------------------------------------------------------------------------
KMHeaders::~KMHeaders ()
{
  if (mFolder)
  {
    writeFolderConfig();
    mFolder->close();
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::readConfig (void)
{
  KConfig* config = app->getConfig();
  QString fntStr;

  config->setGroup("Fonts");
  fntStr = config->readEntry("list-font", "helvetica-medium-r-12");

  // You need kdelibs package newer than 11-06-98 for this:
  setTableFont(kstrToFont(fntStr));
}


//-----------------------------------------------------------------------------
void KMHeaders::readFolderConfig (void)
{
  KConfig* config = app->getConfig();
  assert(mFolder!=NULL);

  config->setGroup("Folder-" + mFolder->name());
  setColumnWidth(1, config->readNumEntry("SenderWidth", 200));
  setColumnWidth(2, config->readNumEntry("SubjectWidth", 270));
  setColumnWidth(3, config->readNumEntry("DateWidth", 300));
  mSortCol = config->readNumEntry("SortColumn", (int)KMMsgList::sfDate);
  mSortDescending = (mSortCol < 0);
  mSortCol = abs(mSortCol);
  mTopItem = config->readNumEntry("Top", 0);
  mCurrentItem = config->readNumEntry("Current", 0);
}


//-----------------------------------------------------------------------------
void KMHeaders::writeFolderConfig (void)
{
  KConfig* config = app->getConfig();
  assert(mFolder!=NULL);

  config->setGroup("Folder-" + mFolder->name());
  config->writeEntry("SenderWidth", columnWidth(1));
  config->writeEntry("SubjectWidth", columnWidth(2));
  config->writeEntry("DateWidth", columnWidth(3));
  config->writeEntry("SortColumn", (mSortDescending ? -mSortCol : mSortCol));
  config->writeEntry("Top", topItem());
  config->writeEntry("Current", currentItem());
}


//-----------------------------------------------------------------------------
void KMHeaders::setFolder (KMFolder *aFolder)
{
  int id;
  QString str;
  bool autoUpd = autoUpdate();
  setAutoUpdate(FALSE);

  if (mFolder && mFolder==aFolder)
  {
    id = currentItem();
    updateMessageList();
    setCurrentMsg(id);
  }
  else
  {
    if (mFolder)
    {
      mFolder->markNewAsUnread();
      writeFolderConfig();
      disconnect(mFolder, SIGNAL(msgHeaderChanged(int)),
		 this, SLOT(msgHeaderChanged(int)));
      disconnect(mFolder, SIGNAL(msgAdded(int)),
		 this, SLOT(msgAdded(int)));
      disconnect(mFolder, SIGNAL(msgRemoved(int)),
		 this, SLOT(msgRemoved(int)));
      disconnect(mFolder, SIGNAL(changed()),
		 this, SLOT(msgChanged()));
      disconnect(mFolder, SIGNAL(statusMsg(const char*)),
		 mOwner, SLOT(statusMsg(const char*)));
      mFolder->close();
    }

    mFolder = aFolder;

    if (mFolder)
    {
      connect(mFolder, SIGNAL(msgHeaderChanged(int)),
	      this, SLOT(msgHeaderChanged(int)));
      connect(mFolder, SIGNAL(msgAdded(int)),
	      this, SLOT(msgAdded(int)));
      connect(mFolder, SIGNAL(msgRemoved(int)),
	      this, SLOT(msgRemoved(int)));
      connect(mFolder, SIGNAL(changed()),
	      this, SLOT(msgChanged()));
      connect(mFolder, SIGNAL(statusMsg(const char*)),
	      mOwner, SLOT(statusMsg(const char*)));
      readFolderConfig();
      mFolder->open();
      sort();
    }

    if (mFolder)
    {
      id = findUnread(TRUE, 0, TRUE);
      if (id >= 0)
	      setMsgRead(id);
	  else
          setMsgRead(mCurrentItem);			
    }
    updateMessageList();

    if (mFolder)
    {
      if (id >= 0)
      {
	setCurrentItem(id);
	makeHeaderVisible();
	updateItem(id, FALSE);
      }
      else
      {
	setTopItem(mTopItem);
	setCurrentItem(mCurrentItem);
	updateItem(mCurrentItem, FALSE);
      }
    }
    else setCurrentItem(0);
  }

  if (mFolder)
  {
    if (stricmp(mFolder->whoField(), "To")==0)
      colList[1]->setName(i18n("Receiver"));
    else colList[1]->setName(i18n("Sender"));

    str.sprintf(i18n("%d Messages, %d unread."),
		mFolder->count(), mFolder->countUnread());
    if (mFolder->isReadOnly()) str += i18n("Folder is read-only.");
    mOwner->statusMsg(str);
  }

  setAutoUpdate(autoUpd);
  if (autoUpd) repaint();
}


//-----------------------------------------------------------------------------
void KMHeaders::msgChanged()
{
  int i = topItem();
  int cur = currentItem();
  if (!autoUpdate()) return;
  updateMessageList();
  setTopItem(i);
  setCurrentMsg(cur);
}


//-----------------------------------------------------------------------------
void KMHeaders::msgAdded(int id)
{
  if (!autoUpdate()) return;
  insertItem("", id);
  msgHeaderChanged(id);
  if (mFolder->name() == "outbox")                            // special case ...
    mOwner->messageView()->setMsg(mFolder->getMsg(id), true); // ... for Edited
}


//-----------------------------------------------------------------------------
void KMHeaders::msgRemoved(int id)
{
  if (!autoUpdate()) return;
  removeItem(id);
}


//-----------------------------------------------------------------------------
QString KMHeaders::msgAsLbxString(KMMsgBase* aMsg) const
{
  QString result(256);
  KMMsgStatus flag;
  QString fromStr, subjStr;

  assert(aMsg!=NULL);

  flag = aMsg->status();
  if (flag == KMMsgStatusQueued || flag == KMMsgStatusSent)
    fromStr = KMMessage::stripEmailAddr(aMsg->to());
  else
    fromStr = KMMessage::stripEmailAddr(aMsg->from());
  if (fromStr.isEmpty()) fromStr = i18n("Unknown");

  subjStr = aMsg->subject();
  if (subjStr.isEmpty()) subjStr = i18n("No Subject");

  result.sprintf("%c\n%.100s\n%.100s\n%.40s", (char)flag,
		 (const char*)fromStr, (const char*)subjStr,
		 (const char*)aMsg->dateStr());
  return result;
}


//-----------------------------------------------------------------------------
void KMHeaders::msgHeaderChanged(int msgId)
{
  KMMsgStatus flag;
  KMMsgBase* mb;
  QString fromStr, subjStr;

  if (msgId<0 || !autoUpdate()) return;

  mb = mFolder->getMsgBase(msgId);
  assert(mb != NULL);

  changeItem(msgAsLbxString(mb), msgId);

  flag = mb->status();
  if (flag==KMMsgStatusNew)
  {
    changeItemColor(darkRed, msgId);
  }
  else if(flag==KMMsgStatusUnread ||
	  flag==KMMsgStatusRead)
  {
    changeItemColor(darkBlue, msgId);
  }
  else changeItemColor(kapp->textColor, msgId);
}


//-----------------------------------------------------------------------------
void KMHeaders::headerClicked(int column)
{
  int idx = currentItem();
  KMMsgBasePtr cur;
  const char* sortStr = "(unknown)";
  QString msg;
  static bool working = FALSE;

  if (working) return;
  working = TRUE;

  kbp->busy();

  if (idx >= 0) cur = (*mFolder)[idx];
  else cur = NULL;

  if (mSortCol == column)
  {
    if (!mSortDescending) mSortDescending = TRUE;
    else
    {
      mSortCol = (int)KMMsgList::sfNone;
      mSortDescending = FALSE;
      sortStr = i18n("order of arrival");
    }
  }
  else
  {
    mSortCol = column;
    mSortDescending = FALSE;
  }

  if (mSortCol==(int)KMMsgList::sfSubject) sortStr = i18n("subject");
  else if (mSortCol==(int)KMMsgList::sfDate) sortStr = i18n("date");
  else if (mSortCol==(int)KMMsgList::sfFrom) sortStr = i18n("sender");
  else if (mSortCol==(int)KMMsgList::sfStatus) sortStr = i18n("status");

  if (mSortDescending) msg.sprintf(i18n("Sorting messages descending by %s"),
				   sortStr);
  else msg.sprintf(i18n("Sorting messages ascending by %s"), sortStr);
  mOwner->statusMsg(msg);

  sort();

  if (cur) idx = mFolder->find(cur);
  else idx = 0;

  setCurrentMsg(idx);
  idx -= 3;
  if (idx < 0) idx = 0;
  setTopItem(idx);

  mOwner->statusMsg(msg);
  kapp->processEvents(200);
  working = FALSE;
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::sortAndShow()
{
  int idx = currentItem();
  KMMsgBasePtr cur;
  static bool working = FALSE;

  if (working) return;
  working = TRUE;

  kbp->busy();
  mFolder->quiet( true );

  if (idx >= 0) cur = (*mFolder)[idx];
  else cur = NULL;

  if ((mSortCol != (int)KMMsgList::sfNone) || (mSortDescending != FALSE)) //Don
    sort();

  if (cur) idx = mFolder->find(cur);
  else idx = 0;

  if (idx < 0) idx = 0;
  setCurrentMsg(idx);

  mFolder->quiet( false );
  kapp->processEvents(200);
  working = FALSE;
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::setMsgStatus (KMMsgStatus status, int msgId)
{
  KMMessage* msg;

  for (msg=getMsg(msgId); msg; msg=getMsg())
    msg->setStatus(status);
}


//-----------------------------------------------------------------------------
void KMHeaders::applyFiltersOnMsg(int /*msgId*/)
{
  KMMessage* msg;
  KMMessageList* msgList = selectedMsgs();
  int idx, cur = firstSelectedMsg(currentItem());

  for (idx=cur, msg=msgList->first(); msg; msg=msgList->next())
    filterMgr->process(msg);

  if (cur > (int)count()) cur = count()-1;
  setCurrentMsg(cur);
}


//-----------------------------------------------------------------------------
void KMHeaders::setMsgRead (int msgId)
{
  KMMessage* msg;
  KMMsgStatus st;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    st = msg->status();
    if (st==KMMsgStatusNew || st==KMMsgStatusUnread ||
	st==KMMsgStatusRead)
    {
      msg->setStatus(KMMsgStatusOld);
    }
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::deleteMsg (int msgId)
{
  if (mFolder != trashFolder)
  {
    // move messages into trash folder
    moveMsgToFolder(trashFolder, msgId);
  }
  else
  {
    // We are in the trash folder -> really delete messages
    moveMsgToFolder(NULL, msgId);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::saveMsg (int msgId)
{
  KMMessage* msg;
  QString str;
  QString fileName = KFileDialog::getSaveFileName(".", "*");

  if (fileName.isEmpty()) return;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    str += "From ???@??? 00:00:00 1997 +0000\n";
    str += msg->asString();
    str += "\n";
  }

  if (kStringToFile(str, fileName, TRUE))
    mOwner->statusMsg(i18n("Message(s) saved."));
  else
    mOwner->statusMsg(i18n("Failed to save message(s)."));
}


//-----------------------------------------------------------------------------
void KMHeaders::resendMsg (int msgId)
{
  KMComposeWin *win;
  KMMessage *msg, *newMsg;

  msg = getMsg(msgId);
  if (!msg) return;

  kbp->busy();
  newMsg = new KMMessage;
  newMsg->fromString(msg->asString());
  newMsg->initHeader();
  newMsg->setTo(msg->to());
  newMsg->setSubject(msg->subject());

  win = new KMComposeWin;
  win->setMsg(newMsg, FALSE);
  win->show();
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::forwardMsg (int msgId)
{
  KMComposeWin *win;
  KMMessage *msg;

  msg = getMsg(msgId);
  if (!msg) return;

  kbp->busy();
  win = new KMComposeWin(msg->createForward());
  win->show();
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::replyToMsg (int msgId)
{
  KMComposeWin *win;
  KMMessage *msg;

  msg = getMsg(msgId);
  if (!msg) return;

  kbp->busy();
  win = new KMComposeWin(msg->createReply(FALSE));
  win->show();
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::replyAllToMsg (int msgId)
{
  KMComposeWin *win;
  KMMessage *msg;

  msg = getMsg(msgId);
  if (!msg) return;

  kbp->busy();
  win = new KMComposeWin(msg->createReply(TRUE));
  win->show();
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::moveMsgToFolder (KMFolder* destFolder, int msgId)
{
  KMMessageList* msgList;
  KMMessage* msg;
  int top, rc, cur = firstSelectedMsg(currentItem());
  bool doUpd;

  kbp->busy();
  top = topItem();

  if (destFolder) {
    if(destFolder->open() != 0)
      return;
  }

  msgList = selectedMsgs(msgId);
  doUpd = (msgList->count() > 1);
  if (doUpd) setAutoUpdate(FALSE);

  for (rc=0, msg=msgList->first(); msg && !rc; msg=msgList->next())
  {
    if (destFolder) {
      // "deleting" messages means moving them into the trash folder
      rc = destFolder->moveMsg(msg);
    }
    else
    {
      // really delete messages that are already in the trash folder
      if (doUpd) removeItem(cur);
      mFolder->removeMsg(msg);
      delete msg;
    }
  }

  if (doUpd)
  {
    setAutoUpdate(TRUE);
    updateMessageList();
  }
  setCurrentMsg(cur);
  setTopItem(top);

  if (destFolder) destFolder->close();
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::copyMsgToFolder (KMFolder* destFolder, int msgId)
{
  KMMessageList* msgList;
  KMMessage *msg, *newMsg;
  int top, rc;

  if (!destFolder) return;

  kbp->busy();
  top = topItem();

  destFolder->open();
  msgList = selectedMsgs(msgId);
  for (rc=0, msg=msgList->first(); msg && !rc; msg=msgList->next())
  {
    newMsg = new KMMessage;
    newMsg->fromString(msg->asString());
    assert(newMsg != NULL);

    destFolder->addMsg(newMsg);
  }
  destFolder->close();

  unmarkAll();
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::setCurrentMsg(int cur)
{
  if (cur >= mFolder->count()) cur = mFolder->count() - 1;
  setCurrentItem(cur, -1);
  makeHeaderVisible();
}


//-----------------------------------------------------------------------------
KMMessageList* KMHeaders::selectedMsgs(int idx)
{
  KMMessage* msg;

  mSelMsgList.clear();
  for (msg=getMsg(idx); msg; msg=getMsg())
    mSelMsgList.append(msg);

  return &mSelMsgList;
}


//-----------------------------------------------------------------------------
int KMHeaders::firstSelectedMsg (int msgId) const
{
  if (msgId<0 || msgId>=numRows()) return 0;
  while (msgId>=0 && itemList[msgId]->isMarked())
    msgId--;
  return (msgId+1);
}


//-----------------------------------------------------------------------------
KMMessage* KMHeaders::getMsg (int msgId)
{
  int i, high;

  if (!mFolder || msgId < -2)
  {
    getMsgIndex = -1;
    return NULL;
  }
  if (msgId >= 0)
  {
    getMsgIndex = msgId;
    getMsgMulti = FALSE;
    return mFolder->getMsg(msgId);
  }

  if (msgId == -1)
  {
    getMsgMulti = TRUE;
    getMsgIndex = currentItem();
    for (i=0,high=numRows(); i<high; i++)
    {
      if (itemList[i]->isMarked())
      {
	getMsgIndex = i;
	break;
      }
    }

    return (getMsgIndex>=0 ? mFolder->getMsg(getMsgIndex) : (KMMessage*)NULL);
  }

  if (getMsgIndex < 0) return NULL;

  if (getMsgMulti)
  {
    for (getMsgIndex++; getMsgIndex<numRows(); getMsgIndex++)
    {
      if (itemList[getMsgIndex]->isMarked())
	return mFolder->getMsg(getMsgIndex);
    }
  }

  getMsgIndex = -1;
  return NULL;
}


//-----------------------------------------------------------------------------
void KMHeaders::nextMessage()
{
  int idx = currentItem();
  if (idx < mFolder->count()) setCurrentMsg(idx+1);
}


//-----------------------------------------------------------------------------
void KMHeaders::prevMessage()
{
  int idx = currentItem();
  if (idx > 0) setCurrentMsg(idx-1);
}


//-----------------------------------------------------------------------------
void KMHeaders::nextMessageMark()
{
  int idx = currentItem();
  if (idx < mFolder->count()) setCurrentMsg(idx+1);
}


//-----------------------------------------------------------------------------
void KMHeaders::prevMessageMark()
{
  int idx = currentItem();
  if (idx > 0) setCurrentMsg(idx-1);
}


//-----------------------------------------------------------------------------
int KMHeaders::findUnread(bool aDirNext, int aStartAt, bool onlyNew)
{
  KMMsgBase* msgBase = NULL;
  int i, idx, cnt;

  if (!mFolder) return -1;
  if (!(cnt=mFolder->count()) > 0) return -1;

  if (aStartAt >= 0) idx = aStartAt;
  else
  {
    idx = currentItem();
    if (aDirNext) idx++;
    else idx--;
  }

  if (aDirNext)
  {
    for (i=idx; i<cnt; i++)
    {
      msgBase = mFolder->getMsgBase(i);
      if (!onlyNew && msgBase && msgBase->isUnread()) break;
      if (onlyNew && msgBase && msgBase->isNew()) break;
    }
  }
  else
  {
    for (i=idx; i>=0; i--)
    {
      msgBase = mFolder->getMsgBase(i);
      if (!onlyNew && msgBase && msgBase->isUnread()) break;
      if (onlyNew && msgBase && msgBase->isNew()) break;
    }
  }
  if (i<cnt && i>=0 && msgBase) return i;
  return -1;
}


//-----------------------------------------------------------------------------
void KMHeaders::nextUnreadMessage()
{
  int i = findUnread(TRUE);
  setCurrentMsg(i);
}


//-----------------------------------------------------------------------------
void KMHeaders::prevUnreadMessage()
{
  int i = findUnread(FALSE);
  setCurrentMsg(i);
}


//-----------------------------------------------------------------------------
void KMHeaders::makeHeaderVisible()
{
  int top, cur;

  cur = currentItem();
  if (cur > topItem() && cur < lastRowVisible()) return;

  top = cur - (lastRowVisible() - (topItem() + 1) >> 1);
  if (top < 0) top = 0;

  setTopItem(top);
}


//-----------------------------------------------------------------------------
void KMHeaders::changeItemPart (char c, int itemIndex, int column)
{
  char str[2];

  str[0] = c;
  str[1] = '\0';

  KMHeadersInherited::changeItemPart((const char*)str, itemIndex, column);
}


//-----------------------------------------------------------------------------
void KMHeaders::highlightMessage(int idx, int/*colId*/)
{
  if (idx < 0 || idx >= numRows()) return;

  kbp->busy();
  mOwner->statusMsg("");
  emit selected(mFolder->getMsg(idx));
  if (idx >= 0) setMsgRead(idx);
  updateItem(idx, FALSE);
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::selectMessage(int idx, int/*colId*/)
{
  if (idx < 0 || idx >= numRows()) return;

  kbp->busy();
  mOwner->statusMsg("");
  emit activated(mFolder->getMsg(idx));
  if (idx >= 0) setMsgRead(idx);
  updateItem(idx, FALSE);
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::updateMessageList(void)
{
  long i;
  KMMsgStatus flag;
  KMMsgBase* mb;
  bool autoUpd;

  clear();
  if (!mFolder)
  {
    repaint();
    return;
  }

  kbp->busy();
  autoUpd = autoUpdate();
  setAutoUpdate(FALSE);

  for (i=0; i<mFolder->count(); i++)
  {
    mb = mFolder->getMsgBase(i);
    assert(mb != NULL); // otherwise using count() above is wrong

    insertItem(msgAsLbxString(mb));

    flag = mb->status();
    if (flag==KMMsgStatusNew)
    {
      changeItemColor(darkRed);
    }
    else if(flag==KMMsgStatusUnread ||
	    flag==KMMsgStatusRead)
    {
      changeItemColor(darkBlue);
    }
    else changeItemColor(kapp->textColor);
  }

  setAutoUpdate(autoUpd);
  if (autoUpd) repaint();

  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::mouseReleaseEvent(QMouseEvent* e)
{
  if (e->button() == RightButton)
  {
    if (mMouseCol >= 0)
    {

    }
  }
  KMHeadersInherited::mouseReleaseEvent(e);
}


//-----------------------------------------------------------------------------
bool KMHeaders :: prepareForDrag (int /*aCol*/, int /*aRow*/, char** data,
				  int* size, int* type)
{
  static KMDragData dd;
  int i, from, to, high;

  high = numRows()-1;
  for (i=0, from=-1; i<=high; i++)
  {
    if (itemList[i]->isMarked())
    {
      from = i;
      break;
    }
  }
  for (i=high, to=-1; i>=0; i--)
  {
    if (itemList[i]->isMarked())
    {
      to = i;
      break;
    }
  }
  if (from < 0 || to < 0) return FALSE;

  dd.init(mFolder, from, to);
  *data = (char*)&dd;
  *size = sizeof(dd);
  *type = DndRawData;

  return TRUE;
}


//-----------------------------------------------------------------------------
void KMHeaders::sort(void)
{
  mFolder->sort((KMMsgList::SortField)mSortCol, mSortDescending);
}


//-----------------------------------------------------------------------------
void KMHeaders::setPalette(const QPalette& p)
{
  QColor c = kapp->windowColor;

  debug("KMHeaders::setPalette(): %d %d %d", c.red(), c.green(), c.blue());

  KMHeadersInherited::setPalette(*kapp->palette());
  lbox.setPalette(p);
  lbox.setBackgroundColor(c);
  lbox.repaint(TRUE);
}


//-----------------------------------------------------------------------------
void KMHeaders::slotRMB(int idx, int colId)
{
  
  //highlightMessage(idx, colId);
  //setCurrentItem(idx);

  if( ! isMarked(idx) ) {
    highlightMessage(idx, colId);
    setCurrentItem(idx);
  }


  if (!topLevelWidget()) return; // safe bet
  
  QPopupMenu *menu = new QPopupMenu;

  if (colId == 0) // popup status menu
  {
    connect(menu, SIGNAL(activated(int)), topLevelWidget(),
            SLOT(slotSetMsgStatus(int)));
    menu->insertItem(i18n("New"), (int)KMMsgStatusNew);
    menu->insertItem(i18n("Unread"), (int)KMMsgStatusUnread);
    menu->insertItem(i18n("Read"), (int)KMMsgStatusOld);
    menu->insertItem(i18n("Replied"), (int)KMMsgStatusReplied);
    menu->insertItem(i18n("Queued"), (int)KMMsgStatusQueued);
    menu->insertItem(i18n("Sent"), (int)KMMsgStatusSent);
  }
  else //else popup message menu
  {
    // no new strings here
    menu->insertItem(i18n("&Reply..."), topLevelWidget(),
                     SLOT(slotReplyToMsg()));
    menu->insertItem(i18n("Reply &All..."), topLevelWidget(),
                     SLOT(slotReplyAllToMsg()));
    menu->insertItem(i18n("&Forward..."), topLevelWidget(),
                     SLOT(slotForwardMsg()), Key_F);
    menu->insertSeparator();
    menu->insertItem(i18n("&Move..."), topLevelWidget(),
                     SLOT(slotMoveMsg()), Key_M);
    menu->insertItem(i18n("&Copy..."), topLevelWidget(),
                     SLOT(slotCopyText()), Key_S);
    menu->insertItem(i18n("&Delete"), topLevelWidget(),
                     SLOT(slotDeleteMsg()), Key_D);
  }

  menu->exec (QCursor::pos(), 0);
  delete menu;
}

//-----------------------------------------------------------------------------

void KMHeaders::removeItem(int itemIndex)
{
  KTabListBox::removeItem (itemIndex);
  // If there's no messages left in the list then the reader window shouldn't
  // be displaying any messages either!
  if (count () < 1)
    mOwner->messageView()->setMsg(0);
}

//-----------------------------------------------------------------------------
#include "kmheaders.moc"
