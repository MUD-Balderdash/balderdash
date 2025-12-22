#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include "merc.h"



const char *ab_day_name[7] =
{
    "Воскресенье",
    "Понедельник",
    "Вторник",
    "Среда",
    "Четверг",
    "Пятница",
    "Суббота"
};
const char *ab_month_name[12] =
{
    "Января",
    "Февраля",
    "Марта",
    "Апреля",
    "Мая",
    "Июня",
    "Июля",
    "Августа",
    "Сентября",
    "Октября",
    "Ноября",
    "Декабря"
};


//[Ссарт]: среда, 15 ЯнварЯ 2003 г. 18:20:22 (MSK)
//static const char format[] = "%s, %2d %s %d г. %.2d:%.2d:%.2d (MSK%s)\n";

// БРУЛЬ
static const char format[] = "%s, %2d %s %d г. %.2d:%.2d:%.2d (UTC%s)\n";


char *__asc_time_r(const struct tm *tp, char *buf);


char *c_time(const time_t *t)
{
    return asc_time(localtime (t));
}

char *asc_time(const struct tm *tp)
{
    static char result[MSL];

    return __asc_time_r(tp, result);
}


char *__asc_time_r(const struct tm *tp, char *buf)
{
    if (tp == NULL)
	return NULL;

    if (sprintf(buf, format,
		(tp->tm_wday < 0 || tp->tm_wday >= 7
		 ? "???" : ab_day_name[tp->tm_wday]),
		tp->tm_mday,
		(tp->tm_mon < 0 || tp->tm_mon >= 12
		 ? "???" : ab_month_name[tp->tm_mon]),
		1900 + tp->tm_year,
		tp->tm_hour, tp->tm_min,
		tp->tm_sec, UTC_OFFSET) < 0)
    {
	return NULL;
    }

    return buf;
}

char get_this_time(void){
	char local_time[10];
	return strftime(local_time, 10, "%T", localtime(&current_time));
}

char get_this_date(void){
	char local_date[20];

	return strftime(local_date, 20, "%Y.%m.%d", localtime(&current_time));
}


/* charset=cp1251 */
