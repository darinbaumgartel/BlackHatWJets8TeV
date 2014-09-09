import os
import sys

# Define the main castor directory
castorhead = "/castor/cern.ch/user/d/dmaitre/BHSNtuples/"

# Get the list of possible castor directories (all 8 TeV, Wplus and Wminus)
print 'Defining 8TeV Blackhat Castor directories...',
PossibleCastorDirs = []
for n in [1,2,3,4,5]:
	for sign in ['p','m']:
		PossibleCastorDirs.append(castorhead+'W'+sign+str(n)+'j')
print len(PossibleCastorDirs),'directories defined.'

# Get the list of all present 8 TeV castor directories
print 'Locating which 8 TeV Castor directories exist...',
def GetCastorDirs():
	directories = os.popen('nsls -R '+castorhead+' | grep ":"').readlines()
	good_dirs = []
	for d in directories:
		if "8TeV" in d:
			d = d.replace('//','/')
			d = d.replace(':','')
			d = d.replace('\n','')
			good_dirs.append(d)
	return good_dirs

PresentCastorDirs = GetCastorDirs()			
print len(PresentCastorDirs),' in total.'

# Define the usable castor directories - the overlap of the Possible and the Present
print 'Determining which subdirectories are relevant...',
UsableCastorDirs = []
for d in PresentCastorDirs:
	for x in PossibleCastorDirs:
		print d,x,x in d
		if x in d:
			if d not in UsableCastorDirs:
				UsableCastorDirs.append(d)
print len(UsableCastorDirs), 'subdirectories should be used.'

# Define the list of subdirectories (where the root files ultimately are)
print 'Determining list of relevant files...'
AllFiles = []
for d in UsableCastorDirs:
	addon = [d+'/'+x.replace('\n','') for x in os.popen('nsls '+d).readlines()]
	for x in addon:
		if '.root' in x:
			AllFiles.append(x)
	print '   -> Registered ',len(AllFiles),'files.'

print 'Writing to FilesToTransfer.txt'

f = open('FilesToTransfer.txt','w')
for afile in AllFiles:
	f.write(afile+'\n')
f.close()
print 'Done.'