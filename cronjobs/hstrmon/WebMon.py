import datetime

def tabMenu( origin = '', level = 0 ):
    lines = []
    lines.append('<table>\n')
    lines.append('<tr><td align=right>Updated: %s, UTC</td></tr>\n' % datetime.datetime.utcnow().ctime())
    lines.append('<tr><td>[')

    basedir = '..'
    if 0 == level:
        basedir = '.'

    head = ''
    tail = ''
    if 'liveTickSumm' == origin:
        head = '<b>'
        tail = '</b>'
    lines.append('<a href=%s/liveTickSumm.html>%sLive Tick Summary%s</a> | ' % (basedir, head, tail))

    head = ''
    tail = ''
    if 'fillratSumm' == origin:
        head = '<b>'
        tail = '</b>'
    lines.append('<a href=%s/fillratSumm.html>%sFill Ratio Summary%s</a> | ' % (basedir, head, tail))

    head = ''
    tail = ''
    if 'hstrTickSumm' == origin:
        head = '<b>'
        tail = '</b>'
    lines.append('<a href=%s/hstrTickSumm.html>%sTick Data History%s</a> | ' % (basedir, head, tail))

    head = ''
    tail = ''
    if 'usBookSumm' == origin:
        head = '<b>'
        tail = '</b>'
    lines.append('<a href=%s/usBookSumm.html>%sUS Book data%s</a> | ' % (basedir, head, tail))

    head = ''
    tail = ''
    if 'tickDataAlerts' == origin:
        head = '<b>'
        tail = '</b>'
    lines.append('<a href=%s/tickDataAlerts.html>%sTick Data Alerts%s</a> ' % (basedir, head, tail))

    lines.append(']</td></tr>')
    lines.append('</table>\n')
    return lines
