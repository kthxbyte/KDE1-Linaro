/**********************************************************************

	--- Qt Architect generated file ---

	File: StringListEditWidgetData.h
	Last generated: Wed Nov 4 12:37:20 1998

	DO NOT EDIT!!!  This file will be automatically
	regenerated by qtarch.  All changes will be lost.

 *********************************************************************/

#ifndef StringListEditWidgetData_included
#define StringListEditWidgetData_included

#include <qwidget.h>
#include <qpushbt.h>
#include <qlined.h>
#include <qlistbox.h>

class StringListEditWidgetData : public QWidget
{
    Q_OBJECT

public:

    StringListEditWidgetData
    (
        QWidget* parent = 0L,
        const char* name = 0L
    );

    virtual ~StringListEditWidgetData();

public slots:


protected slots:

    virtual void downPressed();
    virtual void itemSelected(int);
    virtual void itemChanged(const char*);
    virtual void newItem();
    virtual void deletePressed();
    virtual void upPressed();

protected:
    QListBox* lbStrings;
    QPushButton* buttonUp;
    QPushButton* buttonDown;
    QPushButton* buttonDelete;
    QLineEdit* leLine;
    QPushButton* buttonNew;

};

#endif // StringListEditWidgetData_included
