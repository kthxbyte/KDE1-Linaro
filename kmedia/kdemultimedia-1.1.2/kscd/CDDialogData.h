/**********************************************************************

	--- Dlgedit generated file ---

	File: CDDialogData.h
	Last generated: Thu Jan 1 15:56:19 1998

	DO NOT EDIT!!!  This file will be automatically
	regenerated by dlgedit.  All changes will be lost.

 *********************************************************************/

#ifndef CDDialogData_included
#define CDDialogData_included

#include <qdialog.h>
#include <qlabel.h>
#include <qlined.h>
#include <qlistbox.h>
#include <qpushbt.h>

class CDDialogData : public QDialog
{
    Q_OBJECT

public:

    CDDialogData
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~CDDialogData();

public slots:

    virtual void titleselected(int);

protected slots:


protected:
    QLineEdit* titleedit;
    QListBox* listbox;
    QLineEdit* trackedit;
    QLineEdit* progseq_edit;
    QPushButton* save_button;
    QPushButton* load_button;
    QPushButton* ok_button;
    QPushButton* ext_info_title_button;
    QPushButton* ext_info_button;
    QLabel* disc_id_label;
    QLabel* total_time_label;
    QPushButton* upload_button;

};

#endif // CDDialogData_included