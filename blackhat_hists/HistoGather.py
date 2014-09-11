import os
import sys

files = ['hists/'+x.replace('\n','') for x in os.popen('find hists | grep root | grep -v all').readlines()]

filetypes = []

for f in files:
	forig = str(f)
	f = f.split('/')[-1]
	f = f.replace('real','*')
	f = f.replace('loop','*')
	f = f.replace('born','*')
	f = f.replace('vsub','*')
	for x in range(len(f)-6):
		if f[x:x+4] == 'list':
			numtag =  'list'+f[x+4:x+7]
			if f[x+7]!='_':
				numtag+=f[x+7]
			f = f.replace(numtag,'list*')
	if 'hists/subdir_*/'+f not in filetypes:
		filetypes.append('hists/subdir_*/'+f)
	# print f
# print len(filetypes)
# for f in filetypes:
# 	print f
outfiles = []
combinecommands = []
for f in filetypes:
	outfile = f.replace('.list*','_')
	outfile = outfile.replace('.root','_all.root')
	outfile = outfile.replace('/subdir_*/','/')
	# outfile = outfile.replace('','/')

	for x in range(10):
		outfile = outfile.replace('__','_')
	outfiles.append(outfile)
	# print f
	# print './combineHistograms.exe -outfile '+outfile+' '+f
	combinecommands.append('./combineHistograms.exe -outfile '+outfile+' '+f)
	# os.system('./combineHistograms.exe -outfile '+outfile+' '+f)


gatheredexp = []
for outfile in outfiles:
	gfile = outfile
	# print gfile
	for x in ['V0','R0','I0','B0']:
		gfile = gfile.replace(x,'@@')
	# print gfile
	for x in range(99):
		_x = str(x)
		if len(_x)==1:
			_x = '0'+_x
		nmarker = '@@'+_x+''
		gfile = gfile.replace(nmarker,'*')
	# print gfile
	if gfile not in gatheredexp:
		gatheredexp.append(gfile)
	# print ' '


signhadds = []
for g in gatheredexp:
	outg = g.replace('*_','')
	signhadd = 'hadd '+outg+' '+g
	signhadds.append(signhadd)

coldirs = []
parsedirs = []

for g in gatheredexp:
	outg = g.replace('*_','')
	adir = 'hists/hists_'+outg.split('j_')[-1]
	adir = adir.split('_all')[0]
	if adir not in parsedirs:
		parsedirs.append(adir)
		coldirs.append(adir.replace('.LHgrid',''))

mkdirs = []
for d in coldirs:
	mkdirs.append('mkdir '+d)
finalhadds = []


for _d in range(len(coldirs)):
	d = coldirs[_d]
	dp = parsedirs[_d]
	ahadd = 'hadd '+d+'/'
	for x in ['1j','2j','3j']:
		resfile = 'W'+x+'_all.root '
		rexp = ' '+dp.replace('hists/hists_','hists/W*'+x+'*')+'_*all.root'
		rexp = rexp.replace('.LHgrid','*')
		finalhadds.append(ahadd+resfile+rexp)


do = '--do' in sys.argv

person = os.popen('whoami').readlines()[0].replace('\n','')

def pollcombine():
	output = os.popen('ps aux | grep '+person+' | grep combineHistograms | grep -v grep').readlines()
	nreturn = 0
	for xx in output:
		if 'combineHistograms' in str(xx):
			nreturn += 1
	print '  ---> Polling of jobs found ',nreturn,' combine commands in progress.'
	return nreturn

nn = len(combinecommands)
nnn = 0
for c in combinecommands:
	nnn += 1
	print nnn,'of',nn
	print c + ' & '
	nrunning = pollcombine()
	while nrunning  > 50:
		print 'Too many (',nrunning,') jobs. Pausing...'
		os.system('sleep 10')
		nrunning = pollcombine()

	if do:
		os.system(c+' & ')

nrunning = pollcombine()
while nrunning  > 0:
	print 'Too many (',nrunning,') jobs. Pausing...'
	os.system('sleep 20')
	nrunning = pollcombine()

print ' '
for s in signhadds:
	print s
	if do:
		os.system(s)
print ' '

for m in mkdirs:
	print m
	if do:
		os.system(m)
print ' '
for f in finalhadds:
	print f 
	if do:
		os.system(f)

exit()


os.system('rm -r MergedHistos')
os.system('mkdir MergedHistos')
os.system('mv hists/hists* MergedHistos/')
os.system('mv hists/*all.root MergedHistos/')

