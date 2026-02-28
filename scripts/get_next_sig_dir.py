#!/bin/python
import sys
from os import getcwd, chdir
from string import find

class NextDir:
	models = ['US', 'UF', 'CA', 'EU', 'AT', 'AS', 'AH', 'AA', 'KR']
	fit_desc_reg = 'binSig'
	fit_desc_tevt = 'binSigTevt'
	sig_type_om = 'om'
	sig_type_tm = 'tm'
	last_model_reg = 'KR'
	last_model_tevt = 'AS'
	cwd = ''

	def __init__(self, cwd):
		self.cwd = cwd
		if cwd.find('restar60;0') > 0 or cwd.find('tar600;60_1.0_tar3600;660_0.5') > 0:
			self.fit_desc_reg = 'fit'
			self.fit_desc_tevt = 'fitTevt'
			self.sig_type_om = 'restar60;0'
			self.sig_type_tm = 'tar600;60_1.0_tar3600;660_0.5'
		return

	def get_first_tm_reg(self, d, model):
		next_model = self.models[0]
		d = d.replace('/' + self.sig_type_om, '/' + self.sig_type_tm)
		d = d.replace(model, next_model)
		return d

	def get_first_om_evt(self, d, model):
		next_model = self.models[0]
		d = d.replace('/' + self.fit_desc_reg, '/' + self.fit_desc_tevt)
		d = d.replace('/' + self.sig_type_tm, '/' + self.sig_type_om)
		d = d.replace(model, next_model)
		return d

	def get_first_om_reg(self, d, model):
		next_model = self.models[0]
		d = d.replace('/' + self.fit_desc_tevt, '/' + self.fit_desc_reg)
		d = d.replace(model, next_model)
		return d

	def get_next(self, d, model):
		next_model = self.models[self.models.index(model) + 1]
		d = d.replace(model, next_model)
		return d

	def get_sig_type(self, d):
		sig_type = ''
		if d.find('/' + self.sig_type_om) > 0:
			sig_type = self.sig_type_om
		elif d.find('/' + self.sig_type_tm) > 0:
			sig_type = self.sig_type_tm
		return sig_type

	def get_fit_desc(self, d):
		fit_desc = self.fit_desc_reg
		if d.find(self.fit_desc_tevt) > 0:
			fit_desc = self.fit_desc_tevt
		return fit_desc

	def get_last_model(self, d):
		last_model = self.last_model_reg
		if d.find(self.fit_desc_tevt) > 0:
			last_model = self.last_model_tevt
		return last_model

	def get_model(self, d):
		model = ''
		temp_pos = d.find('hffit2')
		if temp_pos > 0:
			slash_pos = d.find('/', temp_pos)
			model_pos = slash_pos + 1
			model = d[model_pos:(model_pos + 2)]
		return model

	def get_next_working_dir(self):
		nwd = self.cwd

		fit_desc = self.get_fit_desc(self.cwd)
		last_model = self.get_last_model(self.cwd)
		sig_type = self.get_sig_type(self.cwd)
		model = self.get_model(self.cwd)

		if model in self.models:
			if model == last_model:
				if fit_desc == self.fit_desc_reg and sig_type == self.sig_type_om:
					nwd = self.get_first_tm_reg(self.cwd, model)
				elif fit_desc == self.fit_desc_reg and sig_type == self.sig_type_tm:
					nwd = self.get_first_om_evt(self.cwd, model)
				elif fit_desc == self.fit_desc_tevt and sig_type == self.sig_type_om:
					nwd = self.get_first_om_reg(self.cwd, model)
					sys.stderr.write('\nReached the last directory. Continuing from the beginning.\n')
			else:
				nwd = self.get_next(self.cwd, model)
	
		return nwd

nd = NextDir(getcwd())
nwd = nd.get_next_working_dir()
print nwd
