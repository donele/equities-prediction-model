from array import array

def set_range_graphs(grs):
	colors = [1, 2, 4, 6, 8, 3, 5, 7, 9]
	n_colors = len(colors)
	maximum = -1000000.
	minimum = 1000000.
	limit_low = 1000000.
	limit_high = -1000000.
	for gr in grs:
		n = gr.GetN()
		x = gr.GetX()
		for i in range(n):
			if x[i] < limit_low:
				limit_low = x[i]
			if x[i] > limit_high:
				limit_high = x[i]
		y = gr.GetY()
		for i in range(n):
			if y[i] < minimum:
				minimum = y[i]
			if y[i] > maximum:
				maximum = y[i]
	maximum = maximum + 0.15 * abs(maximum - minimum)
	minimum = minimum - 0.15 * abs(maximum - minimum)
	cnt = 0
	for gr in grs:
		gr.SetMinimum(minimum)
		gr.SetMaximum(maximum)
		gr.GetXaxis().SetLimits(limit_low, limit_high)
		gr.SetLineColor(colors[cnt % n_colors]);
		gr.SetLineStyle(cnt / n_colors + 1);
		cnt += 1

def get_ma(v, nma):
	nv = len(v)
	ret = array('d')
	for i in range(nma - 1, nv):
		ma = sum(v[(i - (nma - 1)):(i + 1)], 0.0) / nma
		ret.append(ma)
	return ret

def zx(filename):
    exec(open('%s' % filename).read())
