
#ifndef __CHAN_STREAM_H
#define __CHAN_STREAM_H

#define GET_UINT8(_p1, _offset) *(((uint8 *) _p1) + _offset)
#define GET_UINT16(_p1, _offset) *((uint16 *) (((uint8 *) _p1) + _offset))
#define GET_UINT32(_p1, _offset) *((uint32 *) (((uint8 *) _p1) + _offset))

#define SET_UINT8(_p1, _offset, _value) *(((uint8 *) _p1) + _offset) = _value
#define SET_UINT16(_p1, _offset, _value) *((uint16 *) (((uint8 *) _p1) + _offset)) = _value
#define SET_UINT32(_p1, _offset, _value) *((uint32 *) (((uint8 *) _p1) + _offset)) = _value

void
set_wstr(char* dst, int dstlen, char* src, int srclen);

#endif
