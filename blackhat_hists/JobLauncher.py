import os
import sys
from ROOT import *
import random
allrootfiles = [x.replace('\n','') for x in os.popen('find hists | grep LHgrid | grep root | grep -v all')]
lists = [x.replace('\n','') for x in os.popen('find lists/split | grep "_"')]
randchars = [x for x in '0123456789qwertyuiopsdfghjklzxcvbnm']

print 'Found: ',len(allrootfiles),' root files done.'
nS=500
batchq = '1nd'
for x in range(len(sys.argv)):
	if sys.argv[x]=='-n':
		nS = int(sys.argv[x+1])
	if sys.argv[x]=='-q':
		batchq = str(sys.argv[x+1])

gEnv.GetValue("TFile.Recover", 0)

def IsNotCorruptionTest(afile):
	isGood = True
	f = TFile.Open(afile)
	isGood = '0x(nil)' not in str(f)
	if isGood==True:
		strh=str(f.Get('h_counts'))
		if 'ROOT.TObject' in strh:
			isGood=False
		else:
			nent = f.Get('h_counts').GetEntries()
			# print nent
			isGood = nent>0
		f.Close()
	print isGood
	return isGood

if '--clean' in sys.argv:

	badfiles = []
	n = len(allrootfiles)
	_n = 0
	for a in  allrootfiles:
		_n += 1
		# if _n%1000 == 0:
		print 'checking', _n, 'of',n,':',a
		_isgood = IsNotCorruptionTest(a)

		# print _n, 'of', n,_isgood
		if _isgood==False:
			badfiles.append(a)

	print len(badfiles)
	for b in badfiles:
		print ('rm '+b)
		os.system('rm '+b)

	sys.exit()

allrootfiles = [x.replace('\n','') for x in os.popen('find hists | grep LHgrid | grep root | grep -v all')]
print 'After cleaning: ',len(allrootfiles),' root files done.'


def namesfromlist(_pdf,doerr,r,f):
	pdfmems = [0]
	outnames = []
	if doerr:
		if _pdf=='CT10':
			pdfmems = range(52+1)
		if _pdf=='NNPDF21_100':
			pdfmems = range(99+1)
		if _pdf=='MSTW2008nlo68cl':
			pdfmems = range(40+1)

	for _list in lists:

		for p in pdfmems:
			outname = _list.split('/')[-1]+'_'+_pdf+'.LHgrid_r'+r+'_f'+f+'_m'+str(int(p))+'.root'
			outnames.append(outname)
	return outnames

L = namesfromlist('CT10',False,'1.0','1.0')		

if '--pdfvary' in sys.argv:
	L+= namesfromlist('NNPDF21_100',True,'1.0','1.0')		
	L+= namesfromlist('MSTW2008nlo68cl',True,'1.0','1.0')	
if '--scalevary' in sys.argv:	
	L+= namesfromlist('CT10',False,'2.0','2.0')		
	L+= namesfromlist('CT10',False,'0.5','0.5')		




missingfiles = []
presentrootfiles = [x.split('/')[-1] for x in allrootfiles]

for ll in L:
	if ll not in presentrootfiles:
		missingfiles.append(ll)


hasmistakes = False
print 'Log of mistaken files:'

for ll in presentrootfiles:
	if ll not in L:
		hasmistakes = True
		thefile = os.popen('find hists | grep "'+ll+'"').readlines()[0]
		print 'rm',thefile
if hasmistakes:
	print 'Clean up mistakes before continueing.'
	sys.exit()

print 'There should be: ',len(L),' total root files.'
print 'There are: ',len(missingfiles),' incomplete. Submitting now...'


random.shuffle(missingfiles)


def HundredRandomDirs():
	rdirs = []
	for nn in range(200):
		aranddir = 'hists/subdir_'
		lranddir = 'logs/subdir_'
		for nn in range(20):
			cc = random.choice(randchars)
			aranddir+=cc
			lranddir+=cc
		os.system('mkdir '+aranddir)
		os.system('mkdir '+lranddir)
		rdirs.append(aranddir)
	return rdirs

randdirs = HundredRandomDirs()


pwd = os.popen('pwd').readlines()[0].replace('\n','')


com = './makeHistograms.exe -outfile OUTFILE -pdf UPDF -ren RENSCALE -fac FACSCALE -member MEMBER '

#job = '#!/bin/sh\n\n. /etc/bashrc\n\ncd '+pwd+'\nsource ./rc.bash\n. ./setup.sh\n'
job = '#!/bin/sh\n\ncwd=$(pwd)\n\n. /etc/bashrc\n\ncd '+pwd+'\nsource ./rc.bash\ncd ../\n. ./setup.sh\ncd $cwd\ncp '+pwd+'/makeHistograms.exe .\nexport LHAPATH=/afs/cern.ch/cms/slc6_amd64_gcc472/external/lhapdf/5.8.5/share/lhapdf/PDFsets\n\n'

subcoms = []

def chunks(l, n):
	outchunks = []
	for i in xrange(0, len(l), n):
		outchunks.append(l[i:i+n])
	return outchunks

# if len(missingfiles)>50:
# 	print "Abridging to 50 files."
# 	missingfiles = missingfiles[:50]

chunksize = float(len(missingfiles))/float(nS)
chunksize = round(chunksize)-1
chunksize = int(chunksize)
if chunksize < 1:
	chunksize = 1

cmissingfiles = chunks(missingfiles,chunksize)

print 'Submitting with: ',len(cmissingfiles),' total jobs.'


os.system('rm -r tmpjobs')
os.system('mkdir tmpjobs')
jind = 0

import time

atime = time.clock()

for achunk in cmissingfiles:
	print time.clock()-atime
	atime = time.clock()
	jind += 1
	ajob = str(job)

	tjobname = 'tmpjobs/job_'+str(jind)+'.sh'
	tjob = open(tjobname,'w')
	tjob.write(ajob+'\n\n')


	for m in achunk:
		xyz = 5

		choicedir = random.choice(randdirs)
#		c = com.replace('OUTFILE',choicedir+'/'+m)
		c = com.replace('OUTFILE',pwd+'/'+choicedir+'/'+m)

		_pdf = ''
		if ('CT10' in m):
			_pdf += 'CT10'

		if ('MSTW2008nlo68cl' in m):
			_pdf += 'MSTW2008nlo68cl'

		if 	('NNPDF21_100' in m):
			_pdf += 'NNPDF21_100'

		_pdf +='.LHgrid'

		fend = m.split('LHgrid_')[-1].replace('.root','')
		fend = fend.replace('r','')
		fend = fend.replace('f','')
		fend = fend.replace('m','')
		[_r,_f,_m]  = fend.split('_')

		c = c.replace('UPDF',_pdf)
		c = c.replace('RENSCALE',_r)
		c = c.replace('FACSCALE',_f)
		c = c.replace('MEMBER',_m)

		flist = None
		for _list in lists:
			if _list.split('/')[-1] in m:
				flist = _list


		# _flist = open(flist,'r')
		tjob.write(c)
		for line in open(flist,'r'):
			tjob.write('root://eoscms//eos/cms/store/group/phys_smp/WPlusJets/'+line.replace('\n','')+' ')
		tjob.write('\n\n')
			# c += line.replace('\n','')+' '
		# _flist.close()

		# sflist = ''
		# vfiles = [yy.replace('\n','')+' ' for yy in   os.popen('cat '+flist).readlines()]

		# for y in vfiles:
		# 	sflist += 'root://eoscms//eos/cms/store/group/phys_smp/WPlusJets/'+y.replace('\n','')+' '

	# 	c = c.replace('FLIST',sflist)
	# 	ajob += c+'\n\n'

	tjob.close()

	os.system('chmod 755 '+tjobname)
	bsub = 'bsub -R "pool>15000" -q '+batchq+' -o /dev/null -e /dev/null -J '+tjobname+' < '+tjobname +' '
	print bsub
	if '--do' in sys.argv:
		os.system(bsub) 
		os.system('sleep 1')

