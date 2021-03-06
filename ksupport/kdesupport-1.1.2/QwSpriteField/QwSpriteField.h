// QwSpriteField and associated classes, using Qt C++ class library.
//
// Author: Warwick Allison (warwick@troll.no)
//   Date: 19/10/97
// Copyright (C) 1995-97 by Warwick Allison.
//

#ifndef QwSpriteField_H
#define QwSpriteField_H

#include "Pix.h" // GNU libg++ `Pix' concept (it's just a void*).

#include <qbitmap.h>
#include <qwidget.h>
#include <qscrbar.h>
#include <qlist.h>

class QwSprite;
class QwChunkRec;
class QwSpriteField;

class QwAbsSpriteFieldView
{
public:
    QwAbsSpriteFieldView(QwSpriteField* viewing=0);

    void view(QwSpriteField*);

    virtual QRect viewArea() const=0;
    virtual bool preferDoubleBuffering() const=0;
    virtual void beginPainter(QPainter&)=0;
    virtual void flush(const QRect& area)=0;
    virtual void updateGeometries();

protected:
    QwSpriteField* viewing;
};

class QwSpriteFieldView : public QWidget, public QwAbsSpriteFieldView
{
    Q_OBJECT
public:
    QwSpriteFieldView(QwSpriteField* viewing=0, QWidget* parent=0, const char* name=0, WFlags f=0);
    ~QwSpriteFieldView();

    virtual QRect viewArea() const;
    virtual bool preferDoubleBuffering() const;
    virtual void beginPainter(QPainter&);
    virtual void flush(const QRect& area);

protected:
    // Cause Update for this widget
    virtual void paintEvent(QPaintEvent *);
};

class QwScrollingSpriteFieldView : public QWidget, public QwAbsSpriteFieldView
{
    Q_OBJECT
public:
    QwScrollingSpriteFieldView(QwSpriteField* viewing=0, QWidget* parent=0, const char* name=0, WFlags f=0);
    ~QwScrollingSpriteFieldView();

    virtual QRect viewArea() const;
    virtual bool preferDoubleBuffering() const;
    virtual void beginPainter(QPainter&);
    virtual void flush(const QRect& area);
    virtual void updateGeometries();

protected:
    // Cause Update for this widget
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
    virtual int hSteps() const;
    virtual int vSteps() const;

protected slots:
    void vScroll(int);
    void hScroll(int);

private:
    QPixmap offscr;
    QScrollBar hscroll;
    QScrollBar vscroll;
    int vscrpv;
    int hscrpv;
};

class QwSpriteFieldGraphic
{
public:
    static void setCurrentSpriteField(QwSpriteField*);

    // Alternatively, Graphics can be individually placed on QwSpriteFields.
    void setSpriteField(QwSpriteField*);

    QwSpriteFieldGraphic();
    virtual ~QwSpriteFieldGraphic();

    virtual int z() const=0;

    virtual void draw(QPainter&)=0;

    void show();
    void hide();
    void visible(bool yes);
    bool visible() const; // initially TRUE for QwSpriteFieldGraphics

    // TRUE iff the graphic includes the given pixel position.
    virtual bool at(int x, int y) const=0;
    // TRUE iff the graphic intersects with the given area.
    virtual bool at(const QRect& rect) const=0;
    // TRUE iff the graphic intersects with the given bitmap.
    //   rect gives the offset of the bitmap and relevant area.
    // The default is to just call At(const QRect& rect) above.
    virtual bool at(const QImage* image, const QRect& rect) const;

    virtual int rtti() const;

protected:
    QwSpriteField* spritefield;

    // Visible() will always be TRUE when this is called, as it will
    // be changed before the call when showing and after when hiding.
    // Note, may also be called while moving between QwSpriteFields
    // (see SetSpriteField), such that the QwSpriteFieldGraphic is not visible when
    // moved (conceptually, invisibility is not being on any QwSpriteField).
    virtual void makeVisible(bool yes);

private:
    static QwSpriteField* current_spritefield;
    bool vis;
};

class QwSpriteField
{
public:
    QwSpriteField();
    QwSpriteField(int w, int h, int chunksize=16, int maxclusters=100);
    virtual ~QwSpriteField();

    // Call this after some amount of QwSpriteFieldGraphic motion.
    void update();

    virtual void resize(int width, int height);
    int width() const { return awidth; }
    int height() const { return aheight; }

    int chunkSize() const { return chunksize; }
    void retune(int chunksize, int maxclusters);

    // (x,y) is *world* position, (i,j) is chunk coordinate.

    Pix all();
    Pix topAt(int x, int y);
    Pix lookIn(int x, int y, int w, int h);
    void next(Pix&) const;
    void end(Pix& p) const; // need not be called for p==0
    QwSpriteFieldGraphic* at(Pix p) const;
    bool exact(Pix p) const; // Pre: (p && At(p))
    void protectFromChange(Pix p);

    bool sameChunk(int x1, int y1, int x2, int y2) const
	{ return x1/chunksize==x2/chunksize && y1/chunksize==y2/chunksize; }
    void setChangedChunk(int i, int j);
    void setChangedChunkContaining(int x, int y);

    // These call setChangedChunk
    void addGraphicToChunk(QwSpriteFieldGraphic*, int i, int j);
    void removeGraphicFromChunk(QwSpriteFieldGraphic*, int i, int j);
    void addGraphicToChunkContaining(QwSpriteFieldGraphic*, int x, int y);
    void removeGraphicFromChunkContaining(QwSpriteFieldGraphic*, int x, int y);

    // This is internal.
    void* listAtChunkTopFirst(int i, int j) const;
    void* allList();

    static void setPositionPrecision(int downshifts) { posprec=downshifts; }
    static int positionPrecision() { return posprec; }
    static int world_to_x(int i) { return i>>posprec; }
    static int x_to_world(int i) { return i<<posprec; }

    // These are for QwAbsSpriteFieldView to call
    void addView(QwAbsSpriteFieldView*);
    void removeView(QwAbsSpriteFieldView*);
    void updateInView(QwAbsSpriteFieldView*, const QRect&); // pre: view has been added

    // These are for QwSpriteFieldGraphic to call
    void addGraphic(QwSpriteFieldGraphic*);
    void removeGraphic(QwSpriteFieldGraphic*);

protected:
    virtual void drawBackground(QPainter&, const QRect& area);
    virtual void drawForeground(QPainter&, const QRect& area);

    void forceRedraw(const QRect&);

private:
    QwChunkRec& chunk(int i, int j) const;
    QwChunkRec& chunkContaining(int x, int y) const;

    void drawArea(const QRect&, bool only_changes, QwAbsSpriteFieldView* one_view);

	QPixmap offscr;
    int awidth,aheight;
    int chunksize;
    int maxclusters;
    int chwidth,chheight;
    QwChunkRec* chunks;

    QList<QwAbsSpriteFieldView> viewList;
    QList<QwSpriteFieldGraphic> graphicList;

    static unsigned int posprec;
    static bool double_buffer;
};


class QwSpritePixmap : public QPixmap
{
public:
    QwSpritePixmap(const char* datafilename, const char* maskfilename);
    QwSpritePixmap(const QPixmap&, QPoint hotspot);
    ~QwSpritePixmap();

    int hotX() const { return hotx; }
    int hotY() const { return hoty; }
    void setHotSpot(int x, int y) { hotx = x; hoty = y; }

private:
    friend class QwVirtualSprite;
    friend class QwSpritePixmapSequence;
    friend class QwSpriteFieldIterator;

    int hotx,hoty;

    QImage* collision_mask;
    int colw,colh;
    int colhotx,colhoty;

    QBitmap mask;
};


class QwImageSpriteField : public QwSpriteField
{
public:
    QwImageSpriteField(const char* imagefile, int w, int h, int chunksize=16, int maxclusters=100);
    QwImageSpriteField(QPixmap pm, int w, int h, int chunksize=16, int maxclusters=100);
    virtual ~QwImageSpriteField();

protected:
    virtual void drawBackground(QPainter&, const QRect& area);

private:
    QPixmap image;
};


class QwSpritePixmapSequence
{
public:
    QwSpritePixmapSequence(const char* datafilenamepattern,
	const char* maskfilenamepattern, int framecount=1);
    QwSpritePixmapSequence(QList<QPixmap>, QList<QPoint> hotspots);
    ~QwSpritePixmapSequence();

    void readCollisionMasks(const char* filenamepattern);

    int operator!(); // Failure check.

    QwSpritePixmap* image(int i) const { return img[i]; }
    void setImage(int i, QwSpritePixmap* p) { delete img[i]; img[i]=p; }
    int frameCount() const { return framecount; }

private:
    int framecount;
    QwSpritePixmap** img;
};


class QwVirtualSprite : public QwSpriteFieldGraphic
{
public:
    QwVirtualSprite();

    virtual ~QwVirtualSprite();

    // Reduce collision precision by right-shifting effective coordinates.
    // by the given amount.  Negative values can be used, in which case 
    // the resolution is increased (not often useful).  (default==0)
    //
    static void setPixelCollisionPrecision(int downshifts);

    int width() const;
    int height() const;
    int colWidth() const;
    int colHeight() const;

    // These components must be provided by concrete subclasses.
    // Note the usage of AddToChunks and RemoveFromChunks protected methods.
    //
    virtual QwSpritePixmap* image() const=0;
    virtual int x() const=0;
    virtual int y() const=0;

    virtual bool at(int x, int y) const;
    virtual bool at(const QRect& rect) const;
    virtual bool at(const QImage* image, const QRect& rect) const;

    virtual int rtti() const;

    // Traverse intersecting Graphics.
    //
    // See QwSpriteField::TopAt() for more details.
    //
    Pix neighbourhood() const;
    Pix neighbourhood(int nx, int ny) const;
    Pix neighbourhood(int nx, int ny, QwSpritePixmap*) const;
    //
    void next(Pix&) const;
    void end(Pix& p) const; // need not be called for p==0
    QwSpriteFieldGraphic* at(Pix p) const;
    bool exact(Pix p) const; // Pre: (p && At(p))

    bool hitting(QwSpriteFieldGraphic&) const;
    bool wouldHit(QwSpriteFieldGraphic&, int x, int y, QwSpritePixmap*) const;

    static int world_to_col(int i) { return i>>colprec; }
    static int col_to_world(int i) { return i<<colprec; }

protected:
    virtual void makeVisible(bool yes);

    virtual void draw(QPainter&);

    // Call these either side of X(), Y(), or Image() value changes.
    void addToChunks();
    void removeFromChunks();

    int absX() const;
    int absY() const;
    int absX2() const;
    int absY2() const;

    int absColX() const;
    int absColY() const;
    int absColX2() const;
    int absColY2() const;

    int absX(int nx) const;
    int absY(int ny) const;
    int absX2(int nx) const;
    int absY2(int ny) const;

private:
    static unsigned int colprec;
};

template<class COORD>
class QwPositionedSprite : public QwVirtualSprite
{
public:
    QwPositionedSprite(QwSpritePixmapSequence*);

    QwPositionedSprite();
    void setSequence(QwSpritePixmapSequence* seq);

    virtual ~QwPositionedSprite();

    void frame(int); 
    int frame() const { return frm; }
    int frameCount() const { return images->frameCount(); }

    virtual int x() const { return (int)myx; }
    virtual int y() const { return (int)myy; }
    COORD exact_x() const { return myx; }
    COORD exact_y() const { return myy; }
    void x(COORD);
    void y(COORD);
    void moveBy(COORD dx, COORD dy);
    void moveTo(COORD x, COORD y);
    virtual void moveTo(COORD x, COORD y, int frame);
    virtual int z() const { return alt; }
    void z(int a) { alt=a; changeChunks(); }

    virtual int rtti() const;

    Pix neighbourhood(int frame) const; // Neighbourhood if Frame(frame).
    Pix neighbourhood(int nx, int ny) const { return QwVirtualSprite::neighbourhood(nx,ny); }
    Pix neighbourhood(COORD nx, COORD ny, int frame) const; // Both of above.
    bool wouldHit(QwSpriteFieldGraphic&, COORD x, COORD y, int frame) const;

private:
    COORD myx,myy;
    int frm;
    int alt;

    void changeChunks();

    virtual QwSpritePixmap* image() const { return images->image(frm); }
    QwSpritePixmap* image(int f) const { return images->image(f); }

    QwSpritePixmapSequence* images;
};

template <class COORD>
class QwMobilePositionedSprite : public QwPositionedSprite<COORD> {
public:
    QwMobilePositionedSprite(QwSpritePixmapSequence*);
    QwMobilePositionedSprite();

    void bounds(COORD left, COORD top, COORD right, COORD bottom);
    void adoptSpritefieldBounds();

    enum { Ignore, Stop, Wrap, Bounce };
    void setBoundsAction(int action);
    bool outOfBounds();

    void setVelocity(COORD dX, COORD dY) { dx=dX; dy=dY; }
    COORD dX() const { return dx; }
    COORD dY() const { return dy; }
    void forward(COORD multiplier);
    void forward(COORD multiplier, int frame);

    Pix forwardHits(COORD multiplier) const;
    Pix forwardHits(COORD multiplier, int frame) const;

    void moveTo(COORD x, COORD y) { QwPositionedSprite<COORD>::moveTo(x,y); }
    virtual void moveTo(COORD x, COORD y, int frame);

private:
    int bounds_action;
    COORD dx,dy;
    COORD b_left, b_top, b_right, b_bottom;
    static COORD mod(COORD a,COORD m);
};


// The most common instantiations of the above templates...

class QwSprite : public QwPositionedSprite<int> {
public:
    QwSprite(QwSpritePixmapSequence* s) : QwPositionedSprite<int>(s) { }
    QwSprite() : QwPositionedSprite<int>() { }
};

class QwRealSprite : public QwPositionedSprite<double> {
public:
    QwRealSprite(QwSpritePixmapSequence* s) : QwPositionedSprite<double>(s) { }
    QwRealSprite() : QwPositionedSprite<double>() { }
};

class QwMobileSprite : public QwMobilePositionedSprite<int> {
public:
    QwMobileSprite(QwSpritePixmapSequence* s) : QwMobilePositionedSprite<int>(s) { }
    QwMobileSprite() : QwMobilePositionedSprite<int>() { }
};

class QwRealMobileSprite : public QwMobilePositionedSprite<double> {
public:
    QwRealMobileSprite(QwSpritePixmapSequence* s) : QwMobilePositionedSprite<double>(s) { }
    QwRealMobileSprite() : QwMobilePositionedSprite<double>() { }
};



class QwPolygonalGraphic : public QwSpriteFieldGraphic
{
public:
    QwPolygonalGraphic();
    virtual ~QwPolygonalGraphic();

    bool at(int, int) const;
    bool at(const class QRect &) const;

    int rtti() const;

    QPoint position() const { return pos; }
    int x() const { return pos.x(); }
    int y() const { return pos.y(); }
void z(int a) { alt=a; changeChunks(); }
    int z() const { return alt; }

    void setPen(QPen p);
    void setBrush(QBrush b);

    void moveTo(int x, int y);
    void moveTo(QPoint);
    virtual void moveBy(int dx, int dy);
    void moveBy(QPoint);

protected:
    void makeVisible(bool yes);
    virtual void movingBy(int dx, int dy);
    virtual QPointArray areaPoints() const=0;

    void addToChunks();
    void removeFromChunks();
    void changeChunks();

    void drawRects(QPainter & p);

    void draw(class QPainter &);
    virtual void drawShape(class QPainter &) = 0;

private:
    void chunkify(int);
    bool scan(const QRect&) const;
    QPoint pos;
    int alt;
    QBrush brush;
    QPen pen;
};

class QwRectangle : public QwPolygonalGraphic
{
    int w, h;

public:
    QwRectangle();
    ~QwRectangle();

    int width() const;
    int height() const;
    void setSize(int w, int h);
    int rtti() const;

protected:
    QPointArray areaPoints() const;
    void drawShape(class QPainter &);
};

class QwPolygon : public QwPolygonalGraphic
{
    QPointArray poly;

public:
    QwPolygon();
    ~QwPolygon();
    void setPoints(QPointArray);
    int rtti() const;

protected:
    void movingBy(int dx, int dy);
    QPointArray areaPoints() const;
    void drawShape(class QPainter &);
};

class QwEllipse : public QwPolygonalGraphic
{
    int w, h;
    int a1, a2;

public:
    QwEllipse();
    ~QwEllipse();

    int width() const;
    int height() const;
    void setSize(int w, int h);
    void setAngles(int start, int length);
    int rtti() const;

protected:
    QPointArray areaPoints() const;
    void drawShape(class QPainter &);
};


class QwTextSprite : public QwSpriteFieldGraphic
{
public:
    QwTextSprite();
    QwTextSprite(const char*);
    QwTextSprite(const char*, QFont);

    virtual ~QwTextSprite();

    void setText( const char* );
    void setFont( const QFont& );
    void setColor( const QColor& );

    void x(int);
    void y(int);
    void z(int);
    void moveBy(int dx, int dy);
    void moveTo(int x, int y);

    int x() const { return ox; }
    int y() const { return oy; }
    int z() const;

    int textFlags() const { return flags; }
    void setTextFlags(int);

    const QRect& boundingRect() { return brect; }

    bool at(int, int) const;
    bool at(const class QRect &) const;

    virtual int rtti() const;

protected:
    virtual void makeVisible(bool yes);

    virtual void draw(QPainter&);

    // Call these either side of X(), Y(), or Image() value changes.
    void addToChunks();
    void removeFromChunks();

private:
    void changeChunks();
    void setRect();
    int alt;
    int ox, oy;
    QRect brect;
    QString text;
	int flags;
    QFont font;
    QColor col;
};


#endif
