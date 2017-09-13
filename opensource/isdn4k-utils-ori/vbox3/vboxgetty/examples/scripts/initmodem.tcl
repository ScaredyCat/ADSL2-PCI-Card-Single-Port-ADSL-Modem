# $Id: initmodem.tcl,v 1.2 1998/07/06 09:05:56 michael Exp $
#----------------------------------------------------------------------#
# This script is called every time the modem should be initialized. If #
# the script produce errors (syntax error or the command "error"), the #
# main program interprets the current operation as failed.             #
#                                                                      #
# The following global variables can be used in this script:           #
#                                                                      #
# vbxv_init       - Modem init command (configuration)                 #
# vbxv_initnumber - Modem number init command (configuration)          #
#----------------------------------------------------------------------#
# This script runs under root permissions, so doublecheck what you do! #
#----------------------------------------------------------------------#

if { [vbox_modem_command "$vbxv_init" "OK"] > 0 } {
	if { [vbox_modem_command "$vbxv_initnumber" "OK"] > 0 } {
		if { [vbox_modem_command "AT+FCLASS=8" "OK"] > 0 } {
			if { [vbox_modem_command "ATS13.2=1S13.4=1S13.6=0S13.7=1" "OK"] > 0 } {
				return
			}
		}
	}
}

error "modem returns error"
