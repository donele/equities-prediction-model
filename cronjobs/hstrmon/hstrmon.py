from TickHist import *
import sys

if __name__ == '__main__':

    argc = len(sys.argv)

    if 2 == argc and '-a' == sys.argv[1]:
        alertmaker = AlertMaker()
        alertmaker.run()

    elif 3 == argc and '-c' == sys.argv[1]:
        rrdmaker = RRDMaker()
        d = int(sys.argv[2])
        rrdmaker.create(d)

    elif 3 == argc and '-g' == sys.argv[1]:
        rrdmaker = RRDMaker()
        d = int(sys.argv[2])
        rrdmaker.graph(d)

    elif 2 == argc and '-h' == sys.argv[1]:
        rrdmaker = RRDMaker()
        rrdmaker.html()

    elif 4 == argc and '-u' == sys.argv[1] and len(sys.argv) > 2:
        rrdmaker = RRDMaker()
        d1 = int(sys.argv[2])
        d2 = int(sys.argv[3])
        rrdmaker.update(d1, d2)

    elif 4 == argc and '-r' == sys.argv[1] and len(sys.argv) > 2:
        rrdmaker = RRDMaker()
        d1 = int(sys.argv[2])
        d2 = int(sys.argv[3])
        rrdmaker.create(d2)
        rrdmaker.update(d1, d2)
        rrdmaker.graph(d2)
        rrdmaker.html()

    elif 1 == argc:
        rrdmaker = RRDMaker()
        rrdmaker.create()
        rrdmaker.update()
        rrdmaker.graph()
        rrdmaker.html()
        alertmaker = AlertMaker()
        alertmaker.run()

    else:
        print '\nGive arguments like "-c 20080616", "-u 20070617 20080616", or "-p 20080616"\n'
