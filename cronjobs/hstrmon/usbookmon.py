from USbook import *
import sys

if __name__ == '__main__':

    argc = len(sys.argv)

    plotmaker = PlotMaker()
    plotmaker.plot()
    plotmaker.html()
