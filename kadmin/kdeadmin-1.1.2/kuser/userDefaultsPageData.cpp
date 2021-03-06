/**********************************************************************

	--- Qt Architect generated file ---

	File: userDefaultsPageData.cpp
	Last generated: Thu Dec 10 22:47:22 1998

	DO NOT EDIT!!!  This file will be automatically
	regenerated by qtarch.  All changes will be lost.

 *********************************************************************/

#include <qpixmap.h>
#include <qlayout.h>
#include "userDefaultsPageData.h"
#include <kapp.h>

#define Inherited QWidget

#include <qlabel.h>

userDefaultsPageData::userDefaultsPageData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, 0 )
{
	QLabel* qtarch_Label_11;
	qtarch_Label_11 = new QLabel( this, "Label_11" );
	qtarch_Label_11->setGeometry( 10, 10, 29, 13 );
	qtarch_Label_11->setMinimumSize( 0, 0 );
	qtarch_Label_11->setMaximumSize( 32767, 32767 );
	qtarch_Label_11->setFocusPolicy( QWidget::NoFocus );
	qtarch_Label_11->setBackgroundMode( QWidget::PaletteBackground );
	qtarch_Label_11->setFontPropagation( QWidget::NoChildren );
	qtarch_Label_11->setPalettePropagation( QWidget::NoChildren );
	qtarch_Label_11->setText(i18n("Shell"));
	qtarch_Label_11->setAlignment( 289 );
	qtarch_Label_11->setMargin( -1 );

	shell = new QComboBox( TRUE, this, "ComboBox_1" );
	shell->setGeometry( 10, 40, 160, 30 );
	shell->setMinimumSize( 0, 0 );
	shell->setMaximumSize( 32767, 32767 );
	shell->setFocusPolicy( QWidget::StrongFocus );
	shell->setBackgroundMode( QWidget::PaletteBackground );
	shell->setFontPropagation( QWidget::NoChildren );
	shell->setPalettePropagation( QWidget::NoChildren );
	shell->setInsertionPolicy( QComboBox::AtBottom );
	shell->setSizeLimit( 10 );
	shell->setAutoResize( FALSE );

	QLabel* qtarch_Label_12;
	qtarch_Label_12 = new QLabel( this, "Label_12" );
	qtarch_Label_12->setGeometry( 10, 80, 139, 13 );
	qtarch_Label_12->setMinimumSize( 0, 0 );
	qtarch_Label_12->setMaximumSize( 32767, 32767 );
	qtarch_Label_12->setFocusPolicy( QWidget::NoFocus );
	qtarch_Label_12->setBackgroundMode( QWidget::PaletteBackground );
	qtarch_Label_12->setFontPropagation( QWidget::NoChildren );
	qtarch_Label_12->setPalettePropagation( QWidget::NoChildren );
	qtarch_Label_12->setText(i18n("Base of home directories"));
	qtarch_Label_12->setAlignment( 289 );
	qtarch_Label_12->setMargin( -1 );

	home = new QLineEdit( this, "LineEdit_2" );
	home->setGeometry( 10, 110, 160, 30 );
	home->setMinimumSize( 0, 0 );
	home->setMaximumSize( 32767, 32767 );
	home->setFocusPolicy( QWidget::StrongFocus );
	home->setBackgroundMode( QWidget::PaletteBase );
	home->setFontPropagation( QWidget::NoChildren );
	home->setPalettePropagation( QWidget::NoChildren );
	home->setText( "" );
	home->setMaxLength( 32767 );
	home->setEchoMode( QLineEdit::Normal );
	home->setFrame( TRUE );

	createHomeDir = new QCheckBox( this, "CheckBox_1" );
	createHomeDir->setGeometry( 10, 160, 113, 17 );
	createHomeDir->setMinimumSize( 0, 0 );
	createHomeDir->setMaximumSize( 32767, 32767 );
	createHomeDir->setFocusPolicy( QWidget::TabFocus );
	createHomeDir->setBackgroundMode( QWidget::PaletteBackground );
	createHomeDir->setFontPropagation( QWidget::NoChildren );
	createHomeDir->setPalettePropagation( QWidget::NoChildren );
	createHomeDir->setText(i18n("Create home dir"));
	createHomeDir->setAutoRepeat( FALSE );
	createHomeDir->setAutoResize( TRUE );
	createHomeDir->setChecked( TRUE );

	copySkel = new QCheckBox( this, "CheckBox_2" );
	copySkel->setGeometry( 10, 190, 170, 17 );
	copySkel->setMinimumSize( 0, 0 );
	copySkel->setMaximumSize( 32767, 32767 );
	copySkel->setFocusPolicy( QWidget::TabFocus );
	copySkel->setBackgroundMode( QWidget::PaletteBackground );
	copySkel->setFontPropagation( QWidget::NoChildren );
	copySkel->setPalettePropagation( QWidget::NoChildren );
	copySkel->setText(i18n("Copy skeleton to home dir"));
	copySkel->setAutoRepeat( FALSE );
	copySkel->setAutoResize( TRUE );
	copySkel->setChecked( TRUE );

	userPrivateGroup = new QCheckBox( this, "CheckBox_3" );
	userPrivateGroup->setGeometry( 10, 220, 126, 17 );
	userPrivateGroup->setMinimumSize( 0, 0 );
	userPrivateGroup->setMaximumSize( 32767, 32767 );
	userPrivateGroup->setFocusPolicy( QWidget::TabFocus );
	userPrivateGroup->setBackgroundMode( QWidget::PaletteBackground );
	userPrivateGroup->setFontPropagation( QWidget::NoChildren );
	userPrivateGroup->setPalettePropagation( QWidget::NoChildren );
	userPrivateGroup->setText(i18n("User Private Group"));
	userPrivateGroup->setAutoRepeat( FALSE );
	userPrivateGroup->setAutoResize( TRUE );

	resize( 400,300 );
	setMinimumSize( 0, 0 );
	setMaximumSize( 32767, 32767 );
}

userDefaultsPageData::~userDefaultsPageData()
{
}
