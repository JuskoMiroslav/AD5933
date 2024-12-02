/*
 * CircularBuffer.h
 *
 *  Created on: Oct 30, 2024
 *      Author: Miroslav Jusko
 */

#ifndef INC_CIRCULARBUFFER_H_
#define INC_CIRCULARBUFFER_H_

#include "main.h"
#include <string.h>

template<class T>
class CircularBuffer{
public:
	CircularBuffer(uint16_t size): _size(size),_in_pos(0), _out_pos(0),_stored(0){
		_buffer = new T[_size];
		memset(_buffer,0,_size*sizeof(T));
	}
	~CircularBuffer(void)
	{
		delete[] _buffer;
	}

	void put(T* data,uint16_t size)
	{
		for(uint16_t i = 0;i < size;i++){
			_buffer[_in_pos++] = data[i];
			_stored++;
			if(_in_pos >= _size)
				_in_pos = 0;
			if(_stored >= _size)
				_stored = 0;
		}
	}

	uint16_t pull(T* data,uint16_t size)
		{
		uint16_t read_size = 0;
			for(uint16_t i = 0;i < size;i++){
				if(_stored == 0)
					break;
				data[i] = _buffer[_out_pos++];
				_stored --;

				if(_out_pos>= _size)
					_out_pos = 0;
				read_size ++;
			}
			return read_size;
		}
	uint16_t count(void)
	{
		return _stored;
	}

	void flush(void)
	{
	_in_pos = 0;
	_out_pos = 0;
	_stored = 0;
	memset(_buffer,0,_size*sizeof(T));
	}
private:
	T* _buffer;
	uint16_t _size;
	uint16_t _in_pos;
	uint16_t _out_pos;
	uint16_t _stored;
};



#endif /* INC_CIRCULARBUFFER_H_ */
