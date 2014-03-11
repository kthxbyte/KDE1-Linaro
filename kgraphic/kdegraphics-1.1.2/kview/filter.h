/*
* filter.h -- Declaration of class KImageFilter.
* Generated by newclass on Tue Apr  7 18:21:41 EST 1998.
*/
#ifndef SSK_FILTER_H
#define SSK_FILTER_H

#include<qimage.h>
#include<qobject.h>

/**
* KImageFilter
* @author Sirtaj Singh Kang (taj@kde.org)
* @version $Id: filter.h,v 1.4 1998/10/18 08:17:04 ssk Exp $
*/
class KImageFilter : public QObject
{
private:
	int _max;

	Q_OBJECT
public:
	/**
	* KImageFilter Constructor
	*/
	KImageFilter() : QObject(), _max( 0 ) {}

	virtual void invoke( QImage before ) = 0;

	virtual const char *name() const = 0;

	/**
	* KImageFilter Destructor
	*/
	virtual ~KImageFilter() {}

	virtual KImageFilter *clone() const = 0;

	/**
	* When this is called, the filter should
	* assume a new image has been loaded and
	* should forget its current state (if any);
	*/
	virtual void reset() {}

signals:
	/** 
	 * This is raised when the image is changed.
	 */
	void changed( const QImage& );

	/**
	* Raised when the filter wishes to report progress.
	* @param pct	The percentage of the task completed.
	*/
	void progress( int pct );

	/**
	* To be emitted by filters that wish to inform clients
	* about their current status.
	*/
	void status( const char *status );

	/**
	* Raised when the filter wants to display a temporary message.
	*/
	void message( const char *message );
	
protected:
	void setMaxProgress( int max );
	void setProgress( int val );

private:
	KImageFilter( const KImageFilter& );
	KImageFilter& operator=( const KImageFilter& );
};

#endif // SSK_FILTER_H
