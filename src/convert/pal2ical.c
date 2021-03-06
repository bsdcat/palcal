/* pal2ical
 *
 * Copyright (C) 2004, Scott Kuhl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "../main.h"
#include "../input.h"
#include "../event.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <stdlib.h>

int ical_uid = 0;

/* timezone information */
extern char *tzname[2];

void begin_vevent(PalEvent* event)
{
    g_print("BEGIN:VEVENT\r\n");
    g_print("UID:%d\r\n", ical_uid++);
    g_print("SUMMARY:%s\r\n", event->text);
}

void end_vevent()
{
    g_print("END:VEVENT\r\n");
}

void print_dtstartend(gchar* start_date, PalTime* start_time, PalTime* end_time)
{
    /* if start time is NULL, end_time must be NULL too */
    if(start_time == NULL)
    {
	/* int date_int; */
	g_print("DTSTART;VALUE=DATE:%s\r\n", start_date);

	/* assume the event is an all day event since there are no times for it */
	GDate* d = get_date(start_date);
	g_date_add_days(d, 1);

	g_print("DTEND;VALUE=DATE:%s\r\n", get_key(d));
    }

    else
    {

	g_print("DTSTART;TZID=%s:%sT%02d%02d00\r\n", tzname[0], start_date, start_time->hour, start_time->min);

	g_print("DTEND;TZID=%s:%sT", tzname[0], start_date);
	if(end_time == NULL)
	    g_print("%02d%02d00\r\n", start_time->hour, start_time->min);
	else
	    g_print("%02d%02d00\r\n", end_time->hour, end_time->min);
    }
}


int main(int argc, char* argv[])
{

    char* line;
    FILE* stream;
    int error_count = 0;
    char time_stamp[17];
    time_t time_of_day;

    if(argc == 1)
    {
	g_printerr("pal2ical %s - Copyright (C) 2004, Scott Kuhl\n", PAL_VERSION);
	g_printerr("  %s", "pal is licensed under the GNU General Public License and has NO WARRANTY.\n\n");

	g_printerr("Usage:  pal2ical pal-file.pal > ical-file.ics\n");

	g_printerr("\n");


	exit(1);
    }

    stream = fopen(argv[1],"r");

    if(stream == 0)
    {
	g_printerr("Can't read input file.\n");
	exit(1);
    }

    /* initialize the timezone information (tzname external variable) */
    tzset();
    time(&time_of_day);
    if (strftime(time_stamp, 17, "%Y%m%dT%H%M%SZ", gmtime(&time_of_day)) != 16)
    {
	g_printerr("Can't format timestamp for file.\n");
	exit(1);
    }
    time_stamp[16] = '\0';

    pal_input_skip_comments(stream, NULL);
    PalEvent* head = pal_input_read_head(stream, NULL, argv[1]);
    pal_input_skip_comments(stream, NULL);

    g_print("BEGIN:VCALENDAR\r\n");
    g_print("VERSION:2.0\r\n");
    g_print("CALSCALE:GREGORIAN\r\n");
    g_print("PRODID:-//pal calendar/pal2ical %s//EN\r\n", PAL_VERSION);
    g_print("DTSTAMP:%s\r\n", time_stamp);
    g_print("X-WR-CALNAME;VALUE=TEXT:%s\r\n", head->type);
    
    PalEvent* empty_event = pal_event_init();
    PalEvent* event = pal_input_read_event(stream, NULL, argv[1], empty_event, NULL);

    while(event != NULL)
    {

	/* convert one-time events of format yyyymmdd */
	if(is_valid_yyyymmdd(event->date_string))
	{
	    begin_vevent(event);
	    print_dtstartend(event->date_string, event->start_time, event->end_time);
	    end_vevent();
	}
	else if(is_valid_0000mmdd(event->date_string))
	{
	    begin_vevent(event);

	    if(event->start_date != NULL)
		print_dtstartend(event->start_date, event->start_time, event->end_time);
	    else
		print_dtstartend(g_strconcat("1700", event->date_string+4, NULL), event->start_time, event->end_time);

	    if(event->end_date != NULL)
		g_print("RRULE:FREQ=YEARLY;UNTIL=%s\r\n", get_key(event->end_date));
	    else
		g_print("RRULE:FREQ=YEARLY\r\n");

	    end_vevent();
	}

	else
	{
	    /* ADD MORE STUFF HERE */
	    g_printerr("Can't convert this type event yet: %s %s\n", event->date_string, event->text);
	    error_count++;
	}

	pal_input_skip_comments(stream, NULL);
	event = pal_input_read_event(stream, NULL, argv[1], empty_event, NULL);

    }

    g_print("END:VCALENDAR\r\n");


    g_printerr("DONE!\n\n");
    g_printerr("Events converted: %d\n", ical_uid);
    g_printerr("Events that could not be converted: %d\n", error_count);
    return 0;
}
