// KMail Account Manager

#include "kmacctmgr.h"
#include "kmacctlocal.h"
#include "kmacctpop.h"
#include "kmglobal.h"
#include "kbusyptr.h"
#include "kmfiltermgr.h"

#include <assert.h>
#include <kconfig.h>
#include <kapp.h>
#include <kapp.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

extern KBusyPtr *kbp;
//-----------------------------------------------------------------------------
KMAcctMgr::KMAcctMgr(const char* aBasePath): KMAcctMgrInherited()
{
  assert(aBasePath != NULL);
  mAcctList.setAutoDelete(TRUE);
  setBasePath(aBasePath);
}


//-----------------------------------------------------------------------------
KMAcctMgr::~KMAcctMgr()
{
  writeConfig(FALSE);
  mAcctList.clear();
}


//-----------------------------------------------------------------------------
void KMAcctMgr::setBasePath(const char* aBasePath)
{
  assert(aBasePath != NULL);

  if (aBasePath[0] == '~')
  {
    mBasePath = QDir::homeDirPath();
    mBasePath.append("/");
    mBasePath.append(aBasePath+1);
  }
  else mBasePath = aBasePath;

  mBasePath.detach();
  
}


//-----------------------------------------------------------------------------
void KMAcctMgr::writeConfig(bool withSync)
{
  KConfig* config = app->getConfig();
  KMAccount* acct;
  QString groupName(256);
  int i;

  config->setGroup("General");
  config->writeEntry("accounts", mAcctList.count());

  for (i=1,acct=mAcctList.first(); acct; acct=mAcctList.next(),i++)
  {
    groupName.sprintf("Account %d", i);
    config->setGroup(groupName);
    acct->writeConfig(*config);
  }
  if (withSync) config->sync();
}


//-----------------------------------------------------------------------------
void KMAcctMgr::readConfig(void)
{
  KConfig* config = app->getConfig();
  KMAccount* acct;
  QString groupName(256), acctType, acctName;
  int i, num;

  mAcctList.clear();

  config->setGroup("General");
  num = config->readNumEntry("accounts", 0);

  for (i=1; i<=num; i++)
  {
    groupName.sprintf("Account %d", i);
    config->setGroup(groupName);
    acctType = config->readEntry("Type");
    acctName = config->readEntry("Name");
    acct = create(acctType, acctName);
    if (!acct) continue;
    acct->readConfig(*config);
  }
}


//-----------------------------------------------------------------------------
bool KMAcctMgr::singleCheckMail(KMAccount *account)
{
  debug ("checking mail, server busy");
  serverReady(false);
  bool hasNewMail = FALSE;
  //kbp->busy();
  KMIOStatusWdg *wid = new KMIOStatusWdg(0L,0L,KMIOStatus::RETRIEVE);
  wid->show();

  if (account->folder() == 0)
  {
    QString tmp(1024); //Unsafe
    tmp.sprintf(i18n("Account %s has no mailbox defined!\n"
 	        "Mail checking aborted\n"
	        "Check your account settings!"), account->name().data());
    warning(tmp.data());
  }
  else if (account->processNewMail(wid))
  {
    hasNewMail = TRUE;
    emit newMail(account);
  }
  delete wid;
  filterMgr->cleanup();
  kbp->idle();

  debug ("checked mail, server ready"); // sven
  serverReady(true);                    // sven warning this might be recursive
  // if message "check" is pending. No harm I think.
  return hasNewMail;
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::create(const QString aType, const QString aName) 
{
  KMAccount* act = NULL;

  if (stricmp(aType,"local")==0) 
    act = new KMAcctLocal(this, aName);

  else if (stricmp(aType,"pop")==0) 
    act = new KMAcctPop(this, aName);

  if (act) 
  {
    mAcctList.append(act);
    act->setFolder(inboxFolder);
  }

  return act;
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::find(const QString aName) 
{
  KMAccount* cur;

  if (aName.isEmpty()) return NULL;

  for (cur=mAcctList.first(); cur; cur=mAcctList.next())
  {
    if (cur->name() == aName) return cur;
  }

  return NULL;
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::first(void)
{
  return mAcctList.first();
}


//-----------------------------------------------------------------------------
KMAccount* KMAcctMgr::next(void)
{
  return mAcctList.next();
}


//-----------------------------------------------------------------------------
bool KMAcctMgr::remove(KMAccount* acct)
{
  //assert(acct != NULL);
  if(!acct)
    return FALSE;
  mAcctList.remove(acct);
  return TRUE;
}


//-----------------------------------------------------------------------------
bool KMAcctMgr::checkMail(void)
{
  KMAccount* cur;
  bool hasNewMail = FALSE;

  if (mAcctList.isEmpty())
  {
    warning(i18n("You need to add an account in the network\n"
		 "section of the settings in order to\n"
		 "receive mail."));
    return FALSE;
  }

  debug ("checking mail, server busy");
  serverReady(false);
  KMIOStatusWdg *wid = new KMIOStatusWdg(0L,0L,KMIOStatus::RETRIEVE);
  wid->show();
  
  for (cur=mAcctList.first(); cur; cur=mAcctList.next())
  {
    if (cur->folder() == 0)
    {
      QString tmp(1024); // Unsafe
      tmp.sprintf(i18n("Account %s has no mailbox defined!\n"
                       "Mail checking aborted\n"
                       "Check your account settings!"), cur->name().data());
      warning(tmp.data());
      break;
    }
    else   if (cur->processNewMail(wid))
    {
      hasNewMail = TRUE;
      emit newMail(cur);
    }
  }
  delete wid;
  filterMgr->cleanup();
  debug ("checked mail, server ready");
  serverReady(true);
  return hasNewMail;
}


//-----------------------------------------------------------------------------
QStrList  KMAcctMgr::getAccounts() {
  
  KMAccount *cur;
  QStrList strList;
  for (cur=mAcctList.first(); cur; cur=mAcctList.next()) {
    strList.append(cur->name());
  }

  return strList;

}

//-----------------------------------------------------------------------------
bool KMAcctMgr::intCheckMail(int item) {

  KMAccount* cur;
  bool hasNewMail = FALSE;

  if (mAcctList.isEmpty())
  {
    warning(i18n("You need to add an account in the network\n"
		 "section of the settings in order to\n"
		 "receive mail."));
    return FALSE;
  }
  debug ("checking mail, server busy");
  serverReady(false);
  
  KMIOStatusWdg *wid = new KMIOStatusWdg(0L,0L,KMIOStatus::RETRIEVE);
  wid->show();
  
  int x = 0;
  cur = mAcctList.first();
  for(x=0; x < item; x++)
  {
    cur=mAcctList.next();
  }

  if (cur->folder() == 0)
  {
    QString tmp(1024); // Unsafe
    tmp.sprintf(i18n("Account %s has no mailbox defined!\n"
                     "Mail checking aborted\n"
                     "Check your account settings!"), cur->name().data());
    warning(tmp.data());
  }
  else if (cur->processNewMail(wid))
  {
    hasNewMail = TRUE;
    emit newMail(cur);
  }

  delete wid;
  debug ("checked mail, server ready");
  serverReady(true);
  return hasNewMail;

}


//-----------------------------------------------------------------------------
#include "kmacctmgr.moc"
