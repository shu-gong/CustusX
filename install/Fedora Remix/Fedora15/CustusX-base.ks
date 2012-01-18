###########################################################
# CustusX Fedora 15 Remix Base file
# Include this in a livecd kickstart file. 
# Maintained by Christian Askeland <christian.askeland@sintef.no>
#
# Based on the Omega remix
# This remix is not affliated or endorsed by Red Hat
# Create your own remix - http://fedoraproject.org/wiki/How_to_create_and_use_a_Live_
###########################################################


#######################################
# repositories

repo --name=rpmfusion-free --baseurl=http://download1.rpmfusion.org/free/fedora/releases/15/Everything/$basearch/os
repo --name=rpmfusion-free-updates --baseurl=http://download1.rpmfusion.org/free/fedora/updates/15/$basearch
repo --name=rpmfusion-non-free  --baseurl=http://download1.rpmfusion.org/nonfree/fedora/releases/15/Everything/$basearch/os
repo --name=rpmfusion-non-free-updates --baseurl=http://download1.rpmfusion.org/nonfree/fedora/updates/15/$basearch

##repo --name=livna --baseurl=ftp://mirrors.tummy.com/pub/rpm.livna.org/repo/14/$basearch
#http://rpm.livna.org/livna-release.rpm  
#repo --name=rpmfusion-free     --baseurl=http://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-stable.noarch.rpm  
#repo --name=rpmfusion-non-free --baseurl=http://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-stable.noarch.rpm

# for autoten
### repo --name=fusion --baseurl=http://iso.linux.hr/fusion-linux/fusion-repo/fusion-14/i386/

#######################################

#######################################
# sizing

part iso --size=4998
part / --size=10000


#######################################
%packages
#######################################

#######################################
# Utilities

@office
@admin-tools
@development-libs
@development-tools
@eclipse
@graphical-internet
@sound-and-video

gimp
dia
gedit

#######################################
# Programming stuff, 
# libraries used by CustusX
qt
qt-devel
qt-doc
qt-x11
python
python-devel
python-libs
tk-devel
tcl-devel
graphviz
cppunit-devel
cmake
cmake-gui
eclipse-cdt-sdk
eclipse-pydev
eclipse-cmakeed
eclipse-shelled
freeglut-devel
git
gitk

# v4l (video4linux) - needed for correct operation of OpenCV
libv4l-devel

#custom driver from nvidia
kmod-nvidia


#######################################
# rebranding
-fedora-release
-fedora-release-notes
generic-release
generic-logos
generic-release-notes





#######################################
%post  --log=/root/my-post-log.txt
#######################################


# CustusX stuff

# nice for nvidia installation, given that the kmod is installed (from http://www.fedorafaq.org/#nvidia)
sudo new-kernel-pkg --mkinitrd --dracut --update $(rpm -q --queryformat="%{version}-%{release}.%{arch}\n" kernel | tail -n 1)


#######################################
# add dev user
USER=dev
# generate pw using crypt.h/crypt
/usr/sbin/useradd -m -p CXVDiJJKMFWAU -c "Developer" $USER
/bin/echo "$USER       ALL=(ALL)       NOPASSWD: ALL" >> /etc/sudoers

# reformat prompt to a useful one:
PROMPT='
BLUE="\[\033[1;34m\]"
LIGHT_BLUE="\[\033[0;34m\]"
RED="\[\033[0;31m\]"
GREEN="\[\033[0;32m\]"
LIGHT_RED="\[\033[1;31m\]"
WHITE="\[\033[1;37m\]"
NO_COLOUR="\[\033[0m\]"
TITLEBAR="\[\033]0;\w\007\]"

export PS1="${TITLEBAR}\
$BLUE[$GREEN\$(date +%H:%M:%S) \
$GREEN\w$BLUE]\
$BLUE\$$NO_COLOUR "
PS2="> " # not exported yet
PS4="+ " # not exported yet
'
/bin/echo "$PROMPT" >> /home/${USER}/.bashrc
/bin/echo "Changed prompt in .bashrc"

# set core file size, this defaults to zero on newer Fedora releases
/bin/echo "ulimit -c unlimited" >> /home/${USER}/.bashrc

# configure git
HOME=/home/dev/
/usr/bin/git config --global core.editor "nano"
/usr/bin/git config --global user.name $USER
/usr/bin/git config --global user.email ${USER}@sintef.no
/usr/bin/git config --global color.diff auto
/usr/bin/git config --global color.status auto
/usr/bin/git config --global color.branch auto
/usr/bin/git config --global core.autocrlf input
echo "post git call"

# install rules for ownership of usb devices - important for NDI symlink connections:
# This rule makes usb device usable for ${USER} only.
# See http://reactivated.net/writing_udev_rules.html for other ways to do this.
/bin/echo "KERNEL==\"ttyUSB[0-9]\",		OWNER=\"${USER}\", GROUP=\"${USER}\"" >> 10-CustusX.rules
/bin/mv 10-CustusX.rules /etc/udev/rules.d/

# Required for connection between CustusX and IGSTK.
# This is also performed by the CustusX installer (NA for Linux).
/bin/mkdir -p /Library/CustusX/igstk.links
/bin/chmod a+rwx /Library/CustusX/igstk.links

# test
#cat /usr/share/applications/liveinst.desktop

#######################################
# from Omega

# rebranding

sed -i -e 's/Generic release 14/CustusX Fedora Remix 14/g' /etc/fedora-release /etc/issue
sed -i -e 's/(Generic)/(CustusX-3.2)/g' /etc/fedora-release /etc/issue

#/usr/bin/updatedb
#/usr/sbin/makewhatis

# hide all icons on the desktop

gconftool-2 --direct --config-source=xml:readwrite:/etc/gconf/gconf.xml.defaults -s -t bool /apps/nautilus/desktop/trash_icon_visible false >/dev/null
gconftool-2 --direct --config-source=xml:readwrite:/etc/gconf/gconf.xml.defaults -s -t bool /apps/nautilus/desktop/home_icon_visible false >/dev/null
gconftool-2 --direct --config-source=xml:readwrite:/etc/gconf/gconf.xml.defaults -s -t bool /apps/nautilus/desktop/computer_icon_visible false > /dev/null

# autoten icon on the desktop
### mkdir /etc/skel/Desktop
### cp /usr/share/applications/autoten.desktop /etc/skel/Desktop/

#######################################
%end
#######################################

