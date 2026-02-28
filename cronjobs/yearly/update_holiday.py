import lib_datetime
import string, os, shutil

def update_market(holiday_filename, before_filename, output_filename):
    '''
    Take the before_filename, add holidays from holiday_filename and write to output_filename. 
    '''
    holiday_file = open(holiday_filename, 'r')
    before_file = open(before_filename, 'r')
    print 'Read in %s' % before_filename
    output_file = open(output_filename, 'w')
    holiday_list = map(string.rstrip, holiday_file.readlines()) 
    before_list = map(string.rstrip, before_file.readlines())
    idx_holiday = before_list.index('HOLIDAYS')
    head = before_list[:idx_holiday+1]
    body = before_list[idx_holiday+1:]
    body.extend(holiday_list) 
    # convert to set to remove possible duplicate dates  
    body_set = set(body)
    # convert back to list before sorting
    body = list(body_set)
    body.sort()
    output_file.writelines('\n'.join(head))
    output_file.write('\n')
    output_file.writelines('\n'.join(body))
    output_file.write('\n')
    output_file.close()
    print ' ... Updated %s' % output_filename

def examine_market(before_filename, weekends_list):
    '''
    Check the content of before_filename and make sure it contains dates of Sat and Sun in weekends_list.
    '''
    before_file = open(before_filename, 'r') 
    before_list = map(string.rstrip, before_file.readlines())
    idx_holiday = before_list.index('HOLIDAYS')
    head = before_list[:idx_holiday+1]
    body = before_list[idx_holiday+1:]

    body_set = set(body)
    weekends_set = set(weekends_list)
    if weekends_set.issubset(body_set):
        return True
    else:
        return False

def main():
    # Below two dates need to be updated
    d1 = '20130101'
    d2 = '20131231'
    dates_d1_d2 = lib_datetime.Dates(d1, d2)
    weekends_list = dates_d1_d2.get_weekends()
    
    market_list = ['AU', 'BR', 'CA', 'CH', 'DE', 'DK', 'ES', 'FI', 'FR', 'GB', 'HK', 'IT',
                    'JP', 'KR', 'NASDAQ', 'NO', 'NYSE', 'SE', 'ZA'] 

    # Only US below needs to be updated starting Jan 2016.
    exclude_list = ['EU', 'UNIVERSAL', 'US', 'BRD', 'TW', 'SG'] 
    before_dir = os.path.normpath('L:\\package\\gt\\cronjobs\\yearly\\before')
    output_dir = os.path.normpath('L:\\package\\gt\\cronjobs\\yearly\\output') 
    holiday_dir = os.path.normpath('L:\\package\\gt\\cronjobs\\yearly\\holiday')
    # bookmark the list of old filenames and output filenames
    before_filenames = []
    output_filenames = []
    holiday_filenames = []
    for dirpath, dirnames, filenames in os.walk(before_dir):
        for filename in filenames:
            if filename.endswith('_ex_info.dat'):
                idx = filename.index('_ex_info.dat')
                market = filename[:idx]
                if market in market_list:
                    before_filenames.append(os.path.join(dirpath, filename))
                    output_filenames.append(os.path.join(output_dir, filename))
                    holiday_filename = os.path.join(holiday_dir, '%s_ex_holiday.dat' % market)
                    if os.path.exists(holiday_filename):
                        holiday_filenames.append(holiday_filename)
                    else:
                        raise RuntimeError, '%s does not exist' % holiday_filename
                elif market in exclude_list:
                    print 'file [%s] is not updated, will be copied over' % filename
                else:
                    raise RuntimeError, '%s is not recognized' % filename
            else:
                print 'file [%s] is not a holiday file' % filename
    
    # make sure existing file contains regular weekends already 
    for i in range(len(market_list)):
        if not examine_market(before_filenames[i], weekends_list):
            raise RuntimeError, '%s does not contain regular weekends between %s and %s' % (before_filename[i], d1, d2)

    # update holiday file for each market
    for i in range(len(market_list)):
        update_market(holiday_filenames[i], before_filenames[i], output_filenames[i])

    for market in exclude_list:
        exclude_filename = os.path.join(before_dir, '%s_ex_info.dat' % market)
        shutil.copy(exclude_filename, output_dir)
        print '%s is not updated and copied over' % exclude_filename
        

if __name__ == '__main__':
    '''
    Put original market files in the before directory, put holiday files in holiday 
    directory. Create an empty directory output and write updated files in output.
    '''
    main()
