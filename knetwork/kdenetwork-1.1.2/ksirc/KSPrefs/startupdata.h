/**********************************************************************

	--- Qt Architect generated file ---

	File: startupdata.h
	Last generated: Sat Jan 17 19:44:50 1998

	DO NOT EDIT!!!  This file will be automatically
	regenerated by qtarch.  All changes will be lost.

 *********************************************************************/

#ifndef startupdata_included
#define startupdata_included

#include <qwidget.h>
#include <qpushbt.h>
#include <qlistbox.h>
#include <qlined.h>

class startupdata : public QWidget
{
    Q_OBJECT

public:

    startupdata
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~startupdata();

public slots:


protected slots:

    virtual void nicks_add();
    virtual void nicks_delete();
    virtual void sle_add_update(const char*);

protected:
    QLineEdit* SLE_Nick;
    QLineEdit* SLE_RealName;
    QListBox* LB_Nicks;
    QPushButton* B_AddNicks;
    QLineEdit* SLE_Nicks;

};

#endif // startupdata_included
