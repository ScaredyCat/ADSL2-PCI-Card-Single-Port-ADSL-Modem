# $Id: answercall.tcl,v 1.3 1998/11/10 18:37:05 michael Exp $

#----------------------------------------------------------------------#
# This script is called after the call is answered. Here you can do    #
# message playing, message recording and all other things you want.    #
#                                                                      #
# The following global variables can be used in this script:           #
#                                                                      #
# vbxv_savetime   - The time in unix format the script was started     #
# vbxv_callerid   - The remote's caller ID                             #
# vbxv_callername - The remote's caller name                           #
# vbxv_localphone - The local called phone number                      #
# vbxv_username   - The local user                                     #
# vbxv_userhome   - The local user's spool directory                   #
# vbxv_usedscript - The name of this script :-)                        #
# vbxv_saveulaw   - The name of the voice data record file             #
# vbxv_savevbox   - The name of the voice data description file        #
#----------------------------------------------------------------------#
# The script runs under user permissions!                              #
#----------------------------------------------------------------------#

   # This script is simple and don't use the touchtone feature! If #
   # you want touchtone support, create a new answer script in the #
   # users spool directory and add the touchtone functions!        #

vbox_breaklist c

   # Start voice recording and stop audio playback. If you want a #
   # permanent audio playback, you can start it here. If not, use #
   # vbox to create the temporary vboxctrl-audio control!         #

vbox_voice r start
vbox_voice a stop

    # Now the standard and the beep message are played. The script #
    # will be stopped if the remote caller hangup or if the call   #
    # should be suspended.                                         #

set result [vbox_voice p standard.ulaw beep.ulaw]

if {("$result" != "HANGUP") && ("$result" != "SUSPEND")} {

        # Now the message will be recorded. The script will stop if the #
        # remote caller hangup or if the call should be suspended.      #

    set result [vbox_voice w $vbxv_savetime]
}

    # If the call should be suspended we play a small message to #
    # inform the remote caller.                                  #

if { "$result" == "SUSPEND" } {
    vbox_voice p suspend.ulaw
}

vbox_voice r stop
vbox_voice a stop

    # Uncomment the next lines below and the message will mailed  #
    # to the same user vboxgetty running for.                     #
    #                                                             #
    # Note! I use mutt to send the message via attachment. If you #
    # don't run mutt, use a similar tool or call a script and     #
    # create the attachment manualy!                              #

#if {([file isfile "$vbxv_saveulaw"]) && ([file isfile "$vbxv_savevbox"])} {
#
#    vbox_log A "Mailing recorded message to \"$vbxv_username\"..."
#
#    exec mutt -s "New vbox message ($vbxv_callername)" -a "$vbxv_saveulaw" $vbxv_username <$vbxv_savevbox
#}
