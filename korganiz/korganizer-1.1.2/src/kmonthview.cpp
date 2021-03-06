// 	$Id: kmonthview.cpp,v 1.48.2.1 1999/04/05 20:43:13 glenebob Exp $	

#include "kmonthview.h"
#include "kmonthview.moc"

#include <kapp.h>
#include <kiconloader.h>

#include <qfont.h>
#include <qfontmet.h>
#include <qkeycode.h>

void KNoScrollListBox::keyPressEvent(QKeyEvent *e) 
{
  switch(e->key()) {
  case Key_Right:
    setXOffset(xOffset()+2);
    break; 
  case Key_Left:
    setXOffset(xOffset()-2);
    break;
  case Key_Up:
    if(!count()) break;
    setCurrentItem((currentItem()+count()-1)%count());
    if(!itemVisible(currentItem())) {
      if((unsigned int) currentItem() == (count()-1)) {
	setTopItem(currentItem()-numItemsVisible()+1);
      } else {
	setTopItem(topItem()-1);
      }
    }
    break;
  case Key_Down:
    if(!count()) break;
    setCurrentItem((currentItem()+1)%count());
    if(!itemVisible(currentItem())) {
      if(currentItem() == 0) {
	setTopItem(0);
      } else {
	setTopItem(topItem()+1);
      }
    }
  case Key_Shift:
    emit shiftDown();
    break;
  default:
    break; 
  }
}

void KNoScrollListBox::keyReleaseEvent(QKeyEvent *e) 
{
  switch(e->key()) {
  case Key_Shift:
    emit shiftUp();
    break;
  default:
    break;
  }
}

void KNoScrollListBox::mousePressEvent(QMouseEvent *e)
{
  QListBox::mousePressEvent(e);
  
  if(e->button() == RightButton) {
    emit rightClick();
  } 
}

EventListBoxItem::EventListBoxItem(const char *s)
  : QListBoxItem()
{ 
  setText(s); 
  alarmPxmp = Icon("bell.xpm");
  recurPxmp = Icon("recur.xpm");
}

void EventListBoxItem::paint(QPainter *p)
{
  int x = 3;
  if(recur) {
    p->drawPixmap(x, 0, recurPxmp);
    x += recurPxmp.width()+2;
  }
  if(alarm) {
    p->drawPixmap(x, 0, alarmPxmp);
    x += alarmPxmp.width()+2;
  }
  QFontMetrics fm = p->fontMetrics();
  int yPos;
  int pmheight = QMAX(recurPxmp.height(), alarmPxmp.height());
  if(pmheight < fm.height()) 
    yPos = fm.ascent() + fm.leading()/2;
  else
    yPos = pmheight/2 - fm.height()/2  + fm.ascent();


  // currently disabled because selected items drawn in black!
  //p->setPen(palette().normal().text());
  p->drawText(x, yPos, text());
}

int EventListBoxItem::height(const QListBox *lb) const
{
  return QMAX(recurPxmp.height(), 
	      QMAX(alarmPxmp.height(), lb->fontMetrics().lineSpacing()+1));
}

int EventListBoxItem::width(const QListBox *lb) const
{
  int x = 3;
  if(recur) {
    x += recurPxmp.width()+2;
  }
  if(alarm) {
    x += alarmPxmp.width()+2;
  }
  
  return(x + lb->fontMetrics().width(text())+1);
}

KSummaries::KSummaries(QWidget    *parent, 
		       CalObject  *calendar, 
		       QDate       qd,
		       int         index,
		       const char *name)
  :KNoScrollListBox(parent, name)
{
  idx = index;
  myCal = calendar;
  myDate = qd;

  setFont(QFont("Helvetica", 10));
  currIdxs = new QIntDict<KDPEvent>(101); /* nobody should have more
					     than 101 events on any
					     given day. */
  connect(this, SIGNAL(highlighted(int)), this, SLOT(itemHighlighted(int)));
  connect(this, SIGNAL(selected(int)), this, SLOT(itemSelected(int)));
  calUpdated();
}

void KSummaries::calUpdated()
{
  EventListBoxItem *elitem;
  QString sumstring, ampm, tmpstring;
  KDPEvent *anEvent;
  QTime t1;
  int h;
  unsigned int i;

  setAutoUpdate(FALSE);
  setBackgroundMode(PaletteBase);
  clear();
  currIdxs->clear();
  // 2nd arg is TRUE because we want the events to be sorted.
  events = myCal->getEventsForDate(myDate, TRUE);

  // add new listitems if neccessary.
  for(i = 0, anEvent = events.first();
      anEvent != 0; i++, anEvent = events.next()) {
    t1 = anEvent->getDtStart().time();
    h = t1.hour();
    if(h == 0) {
      h = 12;
      ampm = "a";
    } else if(h > 11) {
      ampm = "p";
      if(h != 12) {
	h -= 12;
      } 
    } else {
      ampm = "a";
    }
    if (anEvent->isMultiDay()) {
      if (myDate == anEvent->getDtStart().date()) {
	sumstring.sprintf("(---- %s", anEvent->getSummary().data());
      } else if (myDate == anEvent->getDtEnd().date()) {
	sumstring.sprintf("%s ----)", anEvent->getSummary().data());
      } else if (!(anEvent->getDtStart().date().daysTo(myDate) % 7)) {
	sumstring.sprintf("---- %s ----", anEvent->getSummary().data());
      } else {
	sumstring.sprintf("----------------");
      }
    } else {
      if (anEvent->doesFloat())
	sumstring = anEvent->getSummary();
      else {
	if (timeAmPm) {
	  sumstring.sprintf("%2d:%02d%s %s", h, t1.minute(), ampm.data(),
			    anEvent->getSummary().data());
	} else {
	  sumstring.sprintf("%2d:%02d %s", t1.hour(), t1.minute(), 
			    anEvent->getSummary().data());
	}
      }
    }

    sumstring.detach();
    elitem = new EventListBoxItem(sumstring);
    elitem->setRecur(anEvent->doesRecur());
    elitem->setAlarm(anEvent->getAlarmRepeatCount() > 0);
    insertItem(elitem);
    currIdxs->insert(i, anEvent);
  }
  setAutoUpdate(TRUE);
  repaint();
}

void KSummaries::setDate(QDate qd)
{
  myDate = qd;
  calUpdated();
}

void KSummaries::itemHighlighted(int index)
{
  itemIndex = index;
  emit daySelected(idx);
}

void KSummaries::itemSelected(int index)
{
    KDPEvent *anEvent;
    
    anEvent = currIdxs->find(index);
    if (!anEvent)
      debug("error, event not found in dictionary");
    else
      emit editEventSignal(anEvent);
}

#define CFRAME_WIDTH 20

KDPMonthView::KDPMonthView(CalObject *calendar,
			   QWidget    *parent, 
			   QDate       qd,
			   const char *name) 
    : QFrame(parent, name)
{
    KIconLoader *loader = kapp->getIconLoader();
    QPixmap pixmap;
    int i;
    
    selDateIdxs.setAutoDelete(TRUE);
    selDates.setAutoDelete(TRUE);
    
    setFrameStyle(WinPanel|Sunken);
    
    vFrame = new QFrame(this);
    vFrame->setFrameStyle(NoFrame);
    vFrame->setBackgroundColor(QColor(black));
    
    cFrame = new QFrame(this);
    cFrame->setFrameStyle(Panel|Raised);
    
    cLayout = new QVBoxLayout(cFrame);
    
    QFont tfont = font();
    tfont.setPointSize(10);
    tfont.setBold(TRUE);
    
    pixmap = loader->loadIcon("3uparrow.xpm");
    upYear = new KPButton(pixmap, cFrame);
    upYear->setFont(tfont);
    cLayout->addWidget(upYear);
    
    pixmap = loader->loadIcon("2uparrow.xpm");
    upMonth = new KPButton(pixmap, cFrame);
    upMonth->setFont(tfont);
    cLayout->addWidget(upMonth);
    
    pixmap = loader->loadIcon("1uparrow.xpm");
    upWeek = new KPButton(pixmap, cFrame);
    upWeek->setFont(tfont);
    cLayout->addWidget(upWeek);
    
    pixmap = loader->loadIcon("1downarrow.xpm");
    downWeek = new KPButton(pixmap, cFrame);
    downWeek->setFont(tfont);
    cLayout->addWidget(downWeek);
    
    pixmap = loader->loadIcon("2downarrow.xpm");
    downMonth = new KPButton(pixmap, cFrame);
    downMonth->setFont(tfont);
    cLayout->addWidget(downMonth);
    
    pixmap = loader->loadIcon("3downarrow.xpm");
    downYear = new KPButton(pixmap, cFrame);
    downYear->setFont(tfont);
    cLayout->addWidget(downYear);
    
    cFrame->setFixedWidth(CFRAME_WIDTH);
    
    connect(upYear,    SIGNAL(clicked()), this, SLOT(goBackYear()));
    connect(upMonth,   SIGNAL(clicked()), this, SLOT(goBackMonth()));
    connect(upWeek,    SIGNAL(clicked()), this, SLOT(goBackWeek()));
    connect(downWeek,  SIGNAL(clicked()), this, SLOT(goForwardWeek()));
    connect(downMonth, SIGNAL(clicked()), this, SLOT(goForwardMonth()));
    connect(downYear,  SIGNAL(clicked()), this, SLOT(goForwardYear()));
    
    myCal = calendar;
    myDate = qd.addDays(-(qd.dayOfWeek()) - 7);

    
    QFont bfont = font();
    bfont.setBold(TRUE);
    
    // make a 13 row, 7 column grid. 1 column for each day of the week,
    // one row for the headers, one row for the headers, and two rows
    // for each of 6 weeks (one for the day header and one for the
    // summaries.
    dayLayout = new QGridLayout(vFrame, 19, 14);
    
    // create the day of the week labels (Sun, Mon, etc) and add them to
    // the layout.
    shortdaynames = TRUE;
    for(i=0; i<7; i++) {
	dayNames[i] = new QLabel(vFrame);
	dayNames[i]->setText("FOO"); // will be replaced by updateConfig();
	dayNames[i]->setFont(bfont);
	dayNames[i]->setFrameStyle(Panel|Raised);
	dayNames[i]->setLineWidth(1);
	dayNames[i]->adjustSize();
	dayNames[i]->setAlignment(AlignCenter);
	dayNames[i]->setMinimumHeight(dayNames[i]->height());
	dayLayout->addMultiCellWidget(dayNames[i], 0, 0, i*2, i*2+1);
	dayLayout->setColStretch(i*2, 1);
	dayLayout->setColStretch(i*2+1, 0);
	dayLayout->addColSpacing(i*2+1, 1);
    }
    
    QDate date = myDate;
    
    // create the days, and add them to the layout.
    for(i=0; i < 42; i++, date = date.addDays(1)) {
	// text is irrelevant here, just needs to be something to set the
	// initial size on (to reduce flicker on initial resize).
	QString daynum;
	daynum.setNum(date.day());
	if(date.day() == 1) {
	    daynum.prepend(" ");
	    daynum.prepend(date.monthName(date.month()));
	}
	dayHeaders[i] = new KSelLabel(vFrame, daynum, i); 
	dayHeaders[i]->setFont(bfont);
	dayHeaders[i]->adjustSize();
	dayHeaders[i]->setMinimumHeight(dayHeaders[i]->height());
	connect(dayHeaders[i], SIGNAL(labelActivated(int)),
		this, SLOT(daySelected(int)));
	connect(dayHeaders[i], SIGNAL(newEventSignal(int)),
		this, SLOT(newEventSlot(int)));
	daySummaries[i] = new KSummaries(vFrame, myCal, date, i);
	daySummaries[i]->setFrameStyle(NoFrame);
	connect(daySummaries[i], SIGNAL(daySelected(int)),
		this, SLOT(daySelected(int)));
	connect(daySummaries[i], SIGNAL(editEventSignal(KDPEvent *)),
		this, SIGNAL(editEventSignal(KDPEvent *)));
	connect(daySummaries[i], SIGNAL(rightClick()),
		this, SLOT(doRightClickMenu()));
	dayLayout->addWidget(dayHeaders[i], i/7*3+1, (i%7)*2);
	dayLayout->addWidget(daySummaries[i], i/7*3+2, (i%7)*2);
    }
    dayLayout->setRowStretch(0, 0);
    for(i=0; i<6; i++) {
	dayLayout->setRowStretch(i*3+1, 0);
	dayLayout->setRowStretch(i*3+2, 1);
	dayLayout->setRowStretch(i*3+3, 0);
	dayLayout->addRowSpacing(i*3+3, 1);
    }
    dispLabel = new QLabel(this);
    dispLabel->setFrameStyle(Panel|Raised);
    dispLabel->setAlignment(AlignCenter);
    QString tstring = (daySummaries[0]->getDate().toString()+" - "+
		       daySummaries[41]->getDate().toString());
    dispLabel->setText(tstring);
    dispLabel->setFont(bfont);
    dispLabel->adjustSize();
    dispLabel->move(2,2);
    
    dayLayout->activate();
    
    cLayout->activate();

    rightClickMenu = new QPopupMenu;
    rightClickMenu->insertItem(i18n("New Event"), this, 
			       SLOT(newEventSelected()));
    rightClickMenu->insertItem(i18n("&Edit"), this, 
			       SLOT(editSelected()));
    rightClickMenu->insertItem(i18n("&Delete"), this, 
			       SLOT(deleteSelected()));

    updateConfig();
}


KDPMonthView::~KDPMonthView()
{
}

KDPEvent *KDPMonthView::getSelected()
{
  int which;

  if (selDateIdxs.count() == 0)
    which = 0;
  else
    which = *selDateIdxs.first();
  
  if (which) {
    return daySummaries[which]->getSelected();
  } else {
    return (KDPEvent *) 0L;
  }
}


void KDPMonthView::updateConfig()
{
  bool fmt;
  const char *longDayNames[] = { i18n("Sunday"), i18n("Monday"), 
				 i18n("Tuesday"), i18n("Wednesday"),
				 i18n("Thursday"),
				 i18n("Friday"), i18n("Saturday") };
  const char *longDayNames2[] = { i18n("Monday"), i18n("Tuesday"),
				  i18n("Wednesday"), i18n("Thursday"),
				  i18n("Friday"), i18n("Saturday"), 
				  i18n("Sunday") };
  
  KConfig *config = kapp->getConfig();
  config->setGroup("Time & Date");
  weekStartsMonday = config->readBoolEntry("Week Starts Monday", FALSE);
  
  fmt = (config->readNumEntry("Time Format", 1) ? TRUE : FALSE);

  QColor tmpColor("#cc3366");
  config->setGroup("Colors");
  QColor hiliteColor = config->readColorEntry("Holiday Color", &tmpColor);

  holidayPalette = palette();
  QColorGroup myGroup = QColorGroup(palette().normal().foreground(),
				    palette().normal().background(),
                                    palette().normal().light(),
                                    palette().normal().dark(),
                                    palette().normal().mid(),
                                    hiliteColor,
                                    palette().normal().base());
  holidayPalette.setNormal(myGroup);
  
  for (int i = 0; i < 42; i++)
    daySummaries[i]->setAmPm(fmt);

  for (int i = 0; i < 7; i++)
    dayNames[i]->setText((weekStartsMonday ? longDayNames2[i] :
			  longDayNames[i]));
  
  // set font
  config->setGroup("Fonts");
  QFont newFont(config->readFontEntry("Month Font", &font()));
  QFont hfont(newFont);
  hfont.setBold(TRUE);
  hfont.setPointSize(newFont.pointSize() + 2);
  for (int i = 0; i < 42; i++) {
    dayHeaders[i]->setFont(hfont);
    daySummaries[i]->setFont(newFont);
  }
  
  viewChanged();
}

void KDPMonthView::goBackYear()
{
  int which;

  if (selDateIdxs.count() == 0)
    which = 0;
  else
    which = *selDateIdxs.first();
  QDate date = daySummaries[which]->getDate();
  date.setYMD(date.year()-1, date.month(), date.day());  
  myDate = date.addDays(-(date.dayOfWeek()));
  while (!myDate.isValid())
    myDate = myDate.addDays(-1);
  viewChanged();
  daySelected(myDate.daysTo(date));
}

void KDPMonthView::goForwardYear()
{
  int which;

  if (selDateIdxs.count() == 0)
    which = 0;
  else
    which = *selDateIdxs.first();
  QDate date = daySummaries[which]->getDate();
  date.setYMD(date.year()+1, date.month(), date.day());  
  myDate = date.addDays(-(date.dayOfWeek()));
  while (!myDate.isValid())
    myDate = myDate.addDays(-1);
  viewChanged();
  daySelected(myDate.daysTo(date));
}

void KDPMonthView::goBackMonth()
{
  int which;

  if (selDateIdxs.count() == 0)
    which = 0;
  else
    which = *selDateIdxs.first();
  QDate date = daySummaries[which]->getDate();
  if (date.month() == 1) {
    date.setYMD(date.year()-1, 12, 1);  
    myDate = date;
    //while (!myDate.isValid())
    //  myDate = myDate.addDays(-1);
  } else {
    date.setYMD(date.year(), date.month()-1, date.day());  
    myDate = date.addDays(-(date.dayOfWeek()));
    while (!myDate.isValid())
      myDate = myDate.addDays(-1);
  }
  myDate = date.addDays(-(date.dayOfWeek()));
  viewChanged();
  daySelected(myDate.daysTo(date));
}

void KDPMonthView::goForwardMonth()
{
  int which;

  if (selDateIdxs.count() == 0)
    which = 0;
  else
    which = *selDateIdxs.first();
  QDate date = daySummaries[which]->getDate();
  if (date.month() == 12) {

    date.setYMD(date.year()+1, 1, 1);
    myDate = date;
    /*
    myDate = date.addDays(-(date.dayOfWeek()));
    while (!myDate.isValid())
    myDate = myDate.addDays(-1);*/
  } else {
      date.setYMD(date.year(), date.month()+1, date.day());  
      myDate = date.addDays(-(date.dayOfWeek()));
      while (!myDate.isValid())
	  myDate = myDate.addDays(-1);
  } 
  myDate = date.addDays(-(date.dayOfWeek()));
  
  viewChanged();
  daySelected(myDate.daysTo(date));
}

void KDPMonthView::goBackWeek()
{
  myDate = myDate.addDays(-7);
  while (!myDate.isValid())
    myDate = myDate.addDays(-1);
  viewChanged();
}

void KDPMonthView::goForwardWeek()
{
  myDate = myDate.addDays(7);
  while (!myDate.isValid())
    myDate = myDate.addDays(-1);
  viewChanged();
}

void KDPMonthView::selectDates(const QDateList dateList)
{
  QDateList tmpList(FALSE);

  tmpList = dateList;

  QDate qd = *(tmpList.first());


  // check to see if we're going to a currently selected date.
  // commented out because it is causing problems where the view isn't
  // refreshed when a different calendar is opened with the same date
  // selected.  bad news...
  /*unsigned int i;
  int *idx;
  for(i=0, idx = selDateIdxs.first(); 
      i < selDateIdxs.count();
      i++, idx = selDateIdxs.next()) {
    if(daySummaries[*idx]->getDate() == qd) return;
    }*/ 
  
  // nope, go to the date.
  if(qd < myDate || qd > myDate.addDays(42)) {
    // the view has to change to accomodate this action.
    myDate = qd.addDays(-(qd.dayOfWeek()) - 7);
  } else {
    // otherwise, just set the seleted day in this view.
    daySelected(myDate.daysTo(qd) - (weekStartsMonday ? 1 : 0));
  }
  viewChanged();
}

void KDPMonthView::changeEventDisplay(KDPEvent *, int)
{
  // this should be re-written to be much more efficient, but this
  // quick-and-dirty-hack gets the job done for right now.
  updateView();
}

void KDPMonthView::viewChanged()
{
  unsigned int i;
  int *idx;
  QDate date = myDate.addDays(weekStartsMonday ? 1 : 0);

  for(i=0; i<42; i++, date = date.addDays(1)) {
    QString daynum;
    daynum.setNum(date.day());
    if(date.day() == 1) {
      daynum.prepend(" ");
      daynum.prepend(date.monthName(date.month()));
    }

    // add holiday, if present
    QString hstring(myCal->getHolidayForDate(date));;
    if (!hstring.isEmpty()) {
      daynum.prepend(" ");
      daynum.prepend(hstring.data());
      dayHeaders[i]->setPalette(holidayPalette);
    } else {
      dayHeaders[i]->setPalette(palette());
    }

    dayHeaders[i]->setText(daynum);
    daySummaries[i]->setDate(date);
    if (date.month() % 2) {
      dayHeaders[i]->setBackgroundMode(PaletteBackground);
      daySummaries[i]->setBackgroundMode(PaletteBackground);
    } else {
      dayHeaders[i]->setBackgroundMode(PaletteLight);
      daySummaries[i]->setBackgroundMode(PaletteLight);
    }
  }
  QString tstring = (daySummaries[0]->getDate().toString()+" - "+
		     daySummaries[41]->getDate().toString());
  dispLabel->setText(tstring);
  for(i=0, idx = selDateIdxs.first(); 
      i < selDateIdxs.count(), idx != 0;
      i++, idx = selDateIdxs.next()) {
    //debug("selDateIdxs.count(): %d",selDateIdxs.count());
    daySelected(*idx);
  }
}

void KDPMonthView::updateView()
{
  int i;
  //  QDate date = myDate.addDays(weekStartsMonday ? 1 : 0);

  for(i=0; i<42; i++/*, date = date.addDays(1)*/) {
    /*    if (date.month() % 2)
      daySummaries[i]->setBackgroundMode(PaletteBackground);
    else
    daySummaries[i]->setBackgroundMode(PaletteLight);*/
    daySummaries[i]->calUpdated();
  }
}

#define SEP     1
#define MARGIN  3
#define BASE_CTRL_BUTTON_WIDTH 20
 
void KDPMonthView::resizeEvent(QResizeEvent *) 
{

  QFontMetrics fontmetric(dayNames[0]->font());
  unsigned int i;

  hide();

  dispLabel->setFixedWidth(width()-4);

  //resize and position the control buttons.
  cFrame->resize(cFrame->width(), height()-dispLabel->height()-2-4);
  vFrame->resize(width()-cFrame->width()-6, height()-dispLabel->height()-2-4);
  vFrame->move(2,dispLabel->geometry().bottomLeft().y()+2);
  cFrame->move(vFrame->geometry().topRight().x()+2,
	       vFrame->geometry().topRight().y());

  // select the appropriate heading string size. "Wednesday" or "Wed".
  // note this only changes the text if the requested size crosses the
  // threshold between big enough to support the full name and not big
  // enough.
  if(dayNames[0]->width() < fontmetric.width("Wednesday")+4 && 
     !shortdaynames) {
      updateConfig();
      for(i=0; i<7; i++) {
	  QString tmpStr = dayNames[i]->text();
	  tmpStr.truncate(3);
	  dayNames[i]->setText(tmpStr.data());
      }
      shortdaynames = TRUE;
  } else if (shortdaynames) {
      updateConfig();
      shortdaynames = FALSE;
  }
  show();
}

void KDPMonthView::daySelected(int index) 
{
  unsigned int i;
  int *idx;
  QDateList dateList;

  for(i=0, idx = selDateIdxs.first();
      i < selDateIdxs.count();
      i++, idx = selDateIdxs.next()) {
    if(*idx != index) {
      dayHeaders[*idx]->setActivated(FALSE);
      daySummaries[*idx]->clearSelection();
    }
  }
  selDateIdxs.clear();
  dayHeaders[index]->setActivated(TRUE);
  daySummaries[index]->setFocus();
  selDateIdxs.append(new int(index));
  
  dateList.setAutoDelete(TRUE);
  dateList.append(new QDate(daySummaries[index]->getDate()));

  emit datesSelected(dateList);
  dateList.clear();

}

void KDPMonthView::newEventSlot(int index)
{
  emit newEventSignal(daySummaries[index]->getDate());
}

void KDPMonthView::doRightClickMenu()
{
  rightClickMenu->popup(QCursor::pos());
}
