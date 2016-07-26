/*	
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * a sample log system	
*/

#ifndef _LOG_H
#define _LOG_H

#include <stdint.h>

typedef struct log *log_t;

void init_log_system();
void close_log_system();

log_t   create_log(const char *filename);

enum{
	LOG_CRIT = 0,
	LOG_ERR,
	LOG_WARNING,
	LOG_NOTICE,
	LOG_INFO,
	LOG_DEBUG,
	LEV_SIZE,
};

void log_write(log_t,uint8_t level, const char *str);

#endif
