/**********************************************************************

	--- Dlgedit generated file ---

	File: IconPathDialogData.h
	Last generated: Tue Jul 15 17:27:58 1997

	DO NOT EDIT!!!  This file will be automatically
	regenerated by dlgedit.  All changes will be lost.

 *********************************************************************/

#ifndef IconPathDialogData_included
#define IconPathDialogData_included

#include <qdialog.h>
#include <qlined.h>
#include <qpushbt.h>

class IconPathDialogData : public QDialog
{
    Q_OBJECT

public:

    IconPathDialogData
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~IconPathDialogData();

public slots:


protected slots:


protected:
    QLineEdit* le_icon_path;
    QPushButton* b_ok;

};

#endif // IconPathDialogData_included
