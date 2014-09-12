#!/usr/bin/env python

#####################################################
# Unix setup script
# Author: Christian Askeland, SINTEF Medical Technology
# Date:   2012.01.19
#
# Description:
#
#             
#################################################             

import subprocess
import optparse
import re
import sys
import os.path
import urllib
import getpass
import platform
import shutil

from cxShell import *
    
class CppBuilder:
    '''
    Contains methods for working on a cpp project
    '''
    def __init__(self):
        self.cmakeOptions={}
    def setPaths(self, base, build, source):
        self.mBasePath = base
        self.mBuildPath = build
        self.mSourcePath = source
    def setControlData(self, data):
        self.controlData = data
    def setBuildType(self, type):
        self.mBuildType = type
    
    def reset(self):
        'delete build folder(s)'
        shell.removeTree(self.mBuildPath)
            
    def build(self):
        self._changeDirToBuild()
        #self._changeDirToBuild()
        if(platform.system() == 'Windows'):
            if(self.controlData.getCMakeGenerator() == 'Eclipse CDT4 - NMake Makefiles'):
                runShell('nmake')
            if(self.controlData.getCMakeGenerator() == 'NMake Makefiles JOM'):
                runShell('''jom -k -j%s''' % str(self.controlData.threads))
        else:
            # the export DYLD... line is a hack to get shared linking to work on MacOS with vtk5.6
            # - http://www.mail-archive.com/paraview@paraview.org/msg07520.html
            # (add it to all project because it does no harm if not needed)
            runShell('''\
    export DYLD_LIBRARY_PATH=`pwd`/bin; \
    make -j%s
    ''' % str(self.controlData.threads))

    def gitClone(self, repository, folder=''):
        self._changeDirToBase()
        runShell('git clone %s %s' % (repository, folder))
        self._changeDirToSource()
        
    def gitSetRemoteURL(self, new_remote_origin_repository, branch):
        self._changeDirToSource()
        runShell('git remote set-url origin %s' % new_remote_origin_repository)
        runShell('git fetch')
        # old (1.7) syntax - update if needed to 'git branch --set-upstream-to origin/<branch>' 
        runShell('git branch --set-upstream %s origin/%s' % (branch, branch))

    def _gitSubmoduleUpdate(self):
        self._changeDirToSource()
        runShell('git submodule sync') # from tsf 
        runShell('git submodule update --init --recursive')                    

    def gitUpdate(self, branch='master', tag=None, submodules=False):
        '''
        pull latest version of branch, include submodules if asked.
        '''
        if tag!=None:
            print "--------- gitUpdate tag [%s], length=%i" % (tag, len(tag))
        else:
            print "--------- gitUpdate tag None"
        if (tag!=None) and (tag!="\"\""):
            self.gitCheckout(tag, submodules=submodules)
            return
        
        self._changeDirToSource()

        runShell('git fetch')
        runShell('git checkout %s' % branch)
        runShell('git pull origin %s' % branch)
        if submodules:
            self._gitSubmoduleUpdate()
        
    def gitCheckout(self, tag, patch=None, submodules=False):
        '''
        Update git to the given tag.
        Skip if HEAD already is at tag.
        If patch is given, apply the patch after updating to tag.
        '''
        self._changeDirToSource()

        checklatest = tag
        if patch:
            checklatest = patch
        if self._checkGitIsAtTag(checklatest):
            return        

        runShell('git fetch')
        runShell('git checkout %s' % tag)
        if submodules:
            self._gitSubmoduleUpdate()
        
        if patch:       
            self._gitApplyPatch(patch)     

    def _checkGitIsAtTag(self, tag):
        output = shell.evaluate('git describe --tags --exact-match')
        if not output:
            return False
        if output.stdout.strip()==tag:
            print "Skipping git update: Tag %s already at HEAD in %s" % (tag, self.mSourcePath)
            return True
        return False
            
    def _gitApplyPatch(self, patchFile):
        '''
            Howto create a patch using git:
            Branch is created like this:
            git checkout v5.8.0
            git branch VTK-5-8-0.patch_branch
            git checkout VTK-5-8-0.patch_branch
            ... make you modifications ...
            git commit -am "message"
            git format-patch master --stdout > VTK-5-8-0.patch
        '''
        self._changeDirToSource()
        branchName = patchFile + "_branch"
        shell.run('git branch -D %s' % branchName, ignoreFailure=True)
        shell.run('git checkout -B %s' % branchName)
        #TODO this can be a bug, if CustusX is not checked out yet, this will not work!!! 
        # (i.e. if patch file is not found in expected position)
        patchPath = self._getPathToModule() + "/.."
        runShell('git am --whitespace=fix --signoff < %s/%s' % (patchPath, patchFile))
        runShell('git tag -f %s' % patchFile) # need this tag to check for change during next update
        
    def _getPathToModule(self):
        # alternatively use  sys.argv[0] ?? 
        moduleFile = os.path.realpath(__file__)
        modulePath = os.path.dirname(moduleFile)
        modulePath = os.path.abspath(modulePath)
        return modulePath
                 
    def makeClean(self):
        self._changeDirToBuild()
        #self._changeDirToBuild()
        if(platform.system() == 'Windows'):
            if(self.controlData.getCMakeGenerator() == 'Eclipse CDT4 - NMake Makefiles'):
                runShell('nmake -clean')
            if(self.controlData.getCMakeGenerator() == 'NMake Makefiles JOM'):
                runShell('jom -clean')
        else:
            runShell('make clean')

    def addCMakeOption(self, key, value):
        self.cmakeOptions[key] = value

    def configureCMake(self, options=''):        
        self._addDefaultCmakeOptions()                
        generator = self.controlData.getCMakeGenerator()
        optionsFromAssembly = self._assembleOptions()
        self._printOptions()        
        cmd = 'cmake -G"%s" %s %s %s'
        cmd = cmd % (generator, options, optionsFromAssembly, self.mSourcePath)        

        self._changeDirToBuild()
        runShell(cmd)

    def _addDefaultCmakeOptions(self):
        add = self.addCMakeOption
        if(platform.system() != 'Windows'):
            add('CMAKE_CXX_FLAGS:STRING', '-Wno-deprecated')
        add('CMAKE_BUILD_TYPE:STRING', self.mBuildType)        
        if self.controlData.m32bit: # todo: add if darwin
            add('CMAKE_OSX_ARCHITECTURES', 'i386')
        add('BUILD_SHARED_LIBS:BOOL', self.controlData.getBuildShared())
        add('CMAKE_ECLIPSE_VERSION', self.controlData.getEclipseVersion())
        add('CMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES', False)            
    
    def _assembleOptions(self):
        return " ".join(["-D%s=%s"%(key,val) for key,val in self.cmakeOptions.iteritems()])
    def _printOptions(self):
        options = "".join(["    %s = %s\n"%(key,val) for key,val in self.cmakeOptions.iteritems()])
        print "*** CMake Options:\n", options

    def _changeDirToBase(self):
        changeDir(self.mBasePath)
    def _changeDirToSource(self):
        changeDir(self.mSourcePath)
    def _changeDirToBuild(self):
        changeDir(self.mBuildPath)
# ---------------------------------------------------------
