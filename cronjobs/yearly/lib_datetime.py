import datetime

class Dates(object):
    def __init__(self, d1, d2):
        '''
        d1 and d2 are strings in the format YYYYMMDD.
        dates contained in this object are inclusive [d1, d2].
        '''
        if int(d1) >= int(d2):
            raise RuntimeError, 'd1 must be earlier than d2'

        self.dates_list = []
        self.weekdays_list = []
        self.weekends_list = []
        day1 = datetime.datetime.strptime(d1, '%Y%m%d')
        day2 = datetime.datetime.strptime(d2, '%Y%m%d')
        oneday = datetime.timedelta(days=1)
        day2_next = day2 + oneday
        while day1 != day2_next:
            day1_str = day1.strftime('%Y%m%d')
            if day1.isoweekday() < 6:
                self.weekdays_list.append(day1_str)
            else:
                self.weekends_list.append(day1_str)
            self.dates_list.append(day1_str)
            day1 += oneday
        
    def get_dates(self):
        return self.dates_list

    def get_weekdays(self):
        return self.weekdays_list

    def get_weekends(self):
        return self.weekends_list
