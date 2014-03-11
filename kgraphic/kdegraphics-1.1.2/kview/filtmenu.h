/*
* filtmenu.h -- Declaration of class KFiltMenuFactory.
* Generated by newclass on Fri Sep 25 05:13:29 EST 1998.
*/
#ifndef SSK_FILTMENU_H
#define SSK_FILTMENU_H

#include<qobject.h>
#include<qlist.h>
#include<qdict.h>
#include<qpopmenu.h>

class KFilterList;
class KImageFilter;

/**
* Creates a cascading menu of filters and provides signalling of
* filter selection from the menu.
* @author Sirtaj Singh Kang (taj@kde.org)
* @version $Id: filtmenu.h,v 1.2 1998/10/08 14:42:46 kulow Exp $
*/
class KFiltMenuFactory : public QObject
{
	Q_OBJECT

public:
	/**
	* KFiltMenuFactory Constructor
	*/
	KFiltMenuFactory( KFilterList *filters, QObject *parent = 0 );

	/**
	* KFiltMenuFactory Destructor
	*/
	virtual ~KFiltMenuFactory();

	/**
	* Returns a popup menu that can be used by the client to
	* allow filter selection. It will be deleted by this class.
	*
	* Multiple calls will return the same menu each time.
	*/
	QPopupMenu *filterMenu();

	KFilterList *filterList();
	
public slots:
	void updateMenu();

signals:
	void selected( KImageFilter *filter );

private slots:
	void raiseFilter( int index );

private:
	KFiltMenuFactory& operator=( KFiltMenuFactory& );
	KFiltMenuFactory( const KFiltMenuFactory& );

	KFilterList	*_filters;
	QPopupMenu	*_menu;
	int		_lastMenuID;
	QList<QPopupMenu> *_menuList;
	QDict<QPopupMenu> *_menuDict;

	QPopupMenu *getItemMenu( QString& name, QPopupMenu *parent, 
			const QString& fullname );
};

inline KFilterList *KFiltMenuFactory::filterList()
{
	return _filters;
}

#endif // SSK_FILTMENU_H