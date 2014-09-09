# Directory where you want the castor directory mirrored.
ndir = '/store/group/phys_smp/WPlusJets/BHSNtuplesV3'

if ndir[-1] != '/':
	ndir += '/'

import os
import sys
import random
from time import sleep
import subprocess, datetime, os, time, signal


CastorDirs = ['/castor/cern.ch/user/d/dmaitre/BHSNtuples/W'+x+'j/8TeV' for x in ['m1','m2','m3','m4','m5','p1','p2','p3','p4','p5']]

def CreateEOSDirs():
	eosdirs = [ndir+'/']
	def getdirsfromfile(afile):
		f = afile.split('BHSNtuples/')[-1]
		f = f.split('/')
		subdirs = []
		thisdir = ndir
		for x in f:
			if x =='/' or '.root' in x:
				continue
			thisdir += x+'/'
			if thisdir not in eosdirs:
				eosdirs.append(thisdir.replace('//','/')) 


	for line in open('FilesToTransfer.txt','r'):
		getdirsfromfile(line)
	eosdirs.sort()
	print eosdirs
	eoscont = str(os.popen('cmsLs -R '+ndir).readlines())
	for e in eosdirs:
		esparse = e.replace('//','/')
		if esparse[-1]=='/':
			esparse = esparse[:-1]
		# print esparse
		# print eoscont
		if esparse not in eoscont:
			print 'cmsMkdir '+e.replace('//','/')
			os.system('cmsMkdir '+e.replace('//','/'))
	return [eosdirs,eoscont]

[eosdirs,eoscont] = CreateEOSDirs()

cpcommands = []

nn = 0
for f in open('FilesToTransfer.txt','r'):
	nn += 1
	e = f.replace('/castor/cern.ch/user/d/dmaitre/BHSNtuples',ndir)
	cp = 'xrdcp "root://castorcms/'+f.replace('\n','')+'" root://eoscms//eos/cms'+e + ' '
	e = e.replace('//','/').replace('\n','')
	if e not in eoscont:

		if '--stage' in sys.argv:
			os.system('stager_get -M '+f.replace('\n','') +' & ')
			os.system('sleep 0.1')
		cpcommands.append(cp)

print len(cpcommands)

if '--stage' in sys.argv:
	sys.exit()

necessarycps = cpcommands


os.system('rm -r BatchJobs')
os.system('mkdir  BatchJobs')

def chunks(l, n):
    return [l[i:i+n] for i in range(0, len(l), n)]

cpgroups = chunks(necessarycps,20)

subs = []

for xx in range(len(cpgroups)):
	f = open('BatchJobs/CP_'+str(xx)+'.tcsh','w')
	subs.append(['BatchJobs/CP_'+str(xx)+'.tcsh','eos_copy_job_'+str(xx)])
	f.write('#!/bin/tcsh\n\n')
	for cp in cpgroups[xx]:
		f.write(cp+'\n\n')
	f.close()

for s in subs:
	os.system('chmod 755 '+s[0])
	if '--submit' in sys.argv:
		os.system('bsub -q 1nh  -e /dev/null -J '+s[1] +' < '+s[0])

	print 'bsub -q 1nh -e /dev/null -J '+s[1] +' < '+s[0]


# print 'Total Files: ',len(castorfiles)
print 'Files requiring copy:',len(necessarycps)
print ' '
if len(necessarycps) == 0:
	print "There are no files which require copying. Transfer is complete!"
