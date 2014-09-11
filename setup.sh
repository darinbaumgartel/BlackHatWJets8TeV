mycwd=`pwd`

cd CMSSW_5_3_20/src
cmsenv
cd -
export PATH=$mycwd/ntuplesreader-1.0-install/bin:${PATH}
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$mycwd/ntuplesreader-1.0-install/lib:/afs/cern.ch/cms/slc6_amd64_gcc472/cms/cmssw/CMSSW_5_3_20/external/slc6_amd64_gcc472/bin/../../../../../../lcg/root/5.32.00-cms21/lib:$mycwd/fastjet-2.4.4-install/lib
export LHAPATH=/afs/cern.ch/cms/slc6_amd64_gcc472/external/lhapdf/5.8.5/share/lhapdf/PDFsets

