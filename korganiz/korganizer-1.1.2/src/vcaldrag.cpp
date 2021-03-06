#include "vcaldrag.h"
#include "vcc.h"

VCalDrag::VCalDrag(VObject *vcal, QWidget *parent, const char *name)
  : QStoredDrag("text/x-vCalendar", parent, name)
{
  char *buf = writeMemVObject(0, 0, vcal);

  QByteArray data;

  data.assign(buf, strlen(buf));
  setEncodedData(data);
  // we don't delete the buf because QByteArray claims it will handle that?!?
}

bool VCalDrag::canDecode(QDragMoveEvent *me)
{
  return me->provides("text/x-vCalendar");
}

bool VCalDrag::decode(QDropEvent *de, VObject **vcal)
{
  QByteArray payload = de->data("text/x-vCalendar");
  if (payload.size()) { // check to see if we got this kind of data
    de->accept();
    
    *vcal = Parse_MIME(payload.data(), payload.size());
    if (*vcal) { // only return true if there was no parse error.
      return TRUE;
    }
  }
  return FALSE;
}

