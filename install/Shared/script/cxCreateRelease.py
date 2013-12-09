#!/usr/bin/env python

#####################################################
# Unix jenkins script
# Author: Christian Askeland, SINTEF Medical Technology
# Date:   2013.09.09
#
# Description:
#
#
#####################################################

import logging
import time    
import subprocess
import sys
import argparse        
import ConfigParser
from jenkinsapi import api
import jenkinsapi

from cx.cxShell import *
from cx.cxPrintFormatter import PrintFormatter
import cx.cxInstallData
import cx.cxComponents
import cx.cxComponentAssembly
import cx.cxCustusXBuilder
import cx.cxBuildScript
#import cx.cxCustusXInstaller
#import cx.cxCustusXTestInstallation
import jenkins # the python-jenkins module - alternative to jenkinsapi with ability to reconfigure job

class Jenkins:
    '''
    Controller for monitoring jenkins and updating
    the power sockets / gummy bears.
    '''
    def __init__(self):
        self.hostname = 'http://christiana-ubuntu-desktop:8080'
        self.username = 'christiana'
        self.jobname = 'CustusX'
        self.password = 'not set'

    def initializeJenkins(self):
        print 'Initializing jenkins python controller ...'
        self._printConfigInfo()
        
        logging.basicConfig(filename='myapp.log', level=logging.DEBUG)
        self.jenkins = api.Jenkins(self.hostname, self.username, self.password)

        self.pyjenkins = jenkins.Jenkins(self.hostname, self.username, self.password)

        print 'jenkins python controller initialized'
        self._printSetupInfo()   
        print '='*40
     
    def getJob(self):
        '''
        Return the active job.
        NOTE:
        Call this for every use, otherwise the 
        build values will not be updated.
        '''
        return self.jenkins.get_job(self.jobname)

    def _printConfigInfo(self):
        ''
        print '  Hostname: ', self.hostname
        print '  Username: ', self.username
        print '  Password: ', self.password

    def _printSetupInfo(self):
        ''
#        print '  Authorization: ', self.jenkins.get_jenkins_auth()
        print '  Available jobs: ', self.jenkins.get_jobs_list()
        print '  Connected to job: ', self.getJob()


class Version:
    '''
    Operates on the file ${source_path}/version.ini.
    Read, modify and write version info.
    Also git tag on the same folder.
    '''
    def __init__(self):
        self.config = ConfigParser.RawConfigParser()
        self.version = {}
        pass
    
    def setSourcePath(self, path):
        self.source_path = path

    def load(self):
        self.config.read(self._getFullFileName())        

    def save(self):
        file = open(self._getFullFileName(), 'w')
        self.config.write(file)        

    def _getFileName(self):
        return 'version.ini'
    def _getFullFileName(self):
        return '%s/%s' % (self.source_path, self._getFileName())

    def __str__(self):
        return 'filename: %s\ncontent: %s' % (self._getFullFileName(), str(self.config._sections))
    
    def _set(self, name, val):
        self.config.set('version', name, val)
    
    def _get(self, name):
        return str(self.config.get('version', name))
    
    def _getint(self, name):
        return self.config.getint('version', name)

    def increase_version(self, increase_type):
        increase_type = increase_type.upper()
        if increase_type=='RELEASE':
            self.increase_full_version()
        elif increase_type=='BETA':
            self.increase_beta_version()
        elif increase_type=='ALPHA':
            self.increase_alpha_version()
        else:
            cxUtilities.assertTrue(False, 'Unknown increase type, failed to release')            

    def increase_full_version(self):
        self._set('minor', self._getint('minor') + 1)
        self._set('patch', 0)
        self._set('type', 'RELEASE')
                
    def increase_beta_version(self):
        self._set('patch', self._getint('patch') + 1)
        self._set('type', 'BETA')
                
    def increase_alpha_version(self):
        self._set('patch', self._getint('patch') + 1)
        self._set('type', 'ALPHA')
    
    def writeGitTag(self):
        tag = self.generateTag() 
        self.sendTagToGit(tag) 

    def generateTag(self):
        elements = [
             self._get('major'), 
             self._get('minor'),
             self._get('patch'), 
             self._getPostfix()]
        retval = 'v%s' % '.'.join(elements)
        return retval
     
    def _getPostfix(self):
        type = self._get('type').upper()
        if type=='RELEASE':
            return ''
        if type=='ALPHA':
            return 'alpha'
        if type=='BETA':
            return 'beta'
        return type
    
    def commitToGit(self):
        shell.changeDir(self.source_path)
        shell.run('git pull')
        shell.run('git add %s' % self._getFileName())
        message = '[script] Updating version file %s to %s' % (self._getFileName(), self.generateTag())
        shell.run('git commit -m "%s"' % message)
        shell.run('git push')

    def sendTagToGit(self, tag):
        message = 'CustusX release %s. Generated by script' % tag

        shell.changeDir(self.source_path)
        shell.run('git tag -a %s -m "%s"' % (tag, message))
        shell.run('git push origin --tags')

        shell.changeDir('%s/data' % self.source_path)
        shell.run('git tag -a %s -m "%s"' % (tag, message))
        shell.run('git push origin --tags')
        


class Controller(cx.cxBuildScript.BuildScript):
    '''
    '''
    def getDescription(self):                  
        return '''\
Jenkins script for creating a new release and publishing it.
             
Release types:
 - RELEASE: 
     Full release. Increases minor number by one, reset patch number to zero.
     Example name: v3.5.0
     
 - BETA: 
     Experimental release. Increases patch number by one. Patch number should always be even.
     Example name: v3.5.2.beta
     
 - ALPHA: 
     Raw release using current HEAD. No git tagging, git SHA and date
     will be added to release text. Patch number should always be odd.
     Example name: v3.5.3.alpha-23.c4aa35.2013-09-05
 
The release ID is written to the version.ini file, and tagged in git.
All non-alpha releases is followed by the creation of an alpha version and tag.

Thus, we get the following pattern:

    v3.5.0        [RELEASE]
    v3.5.1.alpha  autogenerated alpha tag after full release
    ..... development and alpha releases
    v3.6.0        [RELEASE]
    v3.6.1.alpha  autogenerated alpha tag after full release
    ..... development and alpha releases
    v3.6.2.beta   [BETA]
    v3.6.3.alpha  autogenerated alpha tag after beta release
    ..... development and alpha releases
    v3.6.4.beta   [BETA]
    v3.6.5.alpha  autogenerated alpha tag after beta release
    ..... development and alpha releases
    v3.7.0        [RELEASE]
    v3.7.1.alpha  autogenerated alpha tag after full release
    ..... development and alpha releases

'''
        
    def addArgParsers(self):
        self.controlData().setBuildType("Release")
        shell.setRedirectOutput(False)
        
        super(Controller, self).addArgParsers()
        self.additionalParsers.append(self.getArgParser())
       
    def applyArgumentParsers(self, arguments):
        arguments = super(Controller, self).applyArgumentParsers(arguments)
        (self.options, arguments) = self.getArgParser().parse_known_args(arguments)
        print 'Options: ', self.options
        return arguments

    def getArgParser(self):
        p = argparse.ArgumentParser(add_help=False)
        p.add_argument('-r', '--release_type', choices=['release','beta','alpha'], help='Type of release to create, de general description for more.', default='alpha')
        p.add_argument(      '--jenkins_release', action='store_true', default=False, help='Trigger a jenkins release using the generated git tag, publish to release server')
        p.add_argument('-u', '--username', default="user", help='jenkins user')
        p.add_argument('-p', '--password', default="not set", help='jenkins password')
        return p
        
    def getReleaseType(self):
        return self.options.release_type.upper()
    
    def shouldIncreaseVersion(self):
        '''
        alpha releases indicate no tagging - simply generate a release off the head.
        similarly for to input release type: publish head only.
        '''
        return (self.getReleaseType()!='ALPHA') and (self.getReleaseType()!=None)
        
  
    def run(self):
        PrintFormatter.printHeader('Create Release of type %s' % self.getReleaseType(), level=1)

        self._pullFromGit()
      
        if self.shouldIncreaseVersion():
            version = self._loadVersion()
            self._increaseVersion(version, self.getReleaseType())
            publish_tag=version.generateTag()
            self._increaseVersion(version, 'ALPHA')
        else:
            publish_tag = ""            

        if self.options.jenkins_release:
            self.jenkins_publish_release(publish_tag)
            
        self.cxBuilder.finish()
        
    def _getSourcePath(self):
        assembly = self.cxBuilder.assembly                
        custusx = assembly.getComponent(cx.cxComponents.CustusX3)
        return custusx.sourcePath()

    def _pullFromGit(self):
        shell.changeDir(self._getSourcePath())
        shell.run('git checkout master')
        shell.run('git pull')

    def _loadVersion(self):
        retval = Version()
        retval.setSourcePath(self._getSourcePath())
        retval.load()
        PrintFormatter.printInfo('Loaded previous version: %s' % retval.generateTag())
        return retval
    
    def _increaseVersion(self, version, type):
        version.increase_version(type)
        PrintFormatter.printHeader('Increasing version to %s' % version.generateTag(), level=3)
        version.save()
        version.commitToGit();
        version.writeGitTag()
        
    def jenkins_publish_release(self, tag):
        'trigger a jenkins parametrized build that publishes the tagged release'
        PrintFormatter.printHeader('Trigger jenkins build for tag %s' % tag, level=3)

        self.jenkins = Jenkins()
        self.jenkins.username = self.options.username
        self.jenkins.password = self.options.password
        self.jenkins.jobname = 'CustusX_release_ubuntu'
        self.jenkins.initializeJenkins()
        
        parameters = { 'CX_RELEASE_GIT_TAG':tag }
        self.jenkins.jenkins.build_job(self.jenkins.jobname, parameters)
        PrintFormatter.printHeader('Completed triggering the jenkins job %s' % self.jenkins.jobname, level=3)
        pass
    
if __name__ == '__main__':
    controller = Controller()
    controller.run()
