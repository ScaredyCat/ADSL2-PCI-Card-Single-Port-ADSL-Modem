#include "upnp/upnp.h"
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include "config.h"
#include "gatedevice.h"
#include "util.h"
#include "pmlist.h"

int main (int argc, char** argv)
{
	int ret = UPNP_E_SUCCESS;
	int signal;	
	char descDocUrl[50];
	sigset_t sigsToCatch, oldSet;
	pid_t pid,sid;

	if (argc != 3)
   {
      printf("Usage: upnpd <external ifname> <internal ifname>\n");
      printf("Example: upnpd ppp0 eth0\n");
      printf("Example: upnpd eth1 eth0\n");
      exit(0);
   }
	
	// Save the interface names for later uses
	strcpy(extInterfaceName, argv[1]);
	strcpy(intInterfaceName, argv[2]);
		
	// Get the internal ip address to start the daemon on
	GetIpAddressStr(intIpAddress, intInterfaceName);	

	// Put igd in the background as a daemon process.
/*	pid = fork();
	printf("fork\n");
	if (pid < 0)
	{
		perror("Error forking a new process.");
		exit(EXIT_FAILURE);
	}
	if (pid > 0)
		exit(EXIT_SUCCESS);
	if ((sid = setsid()) < 0)
	{
		perror("Error running setsid");
		exit(EXIT_FAILURE);
	}
	if (pid !=0)
	{
		exit(EXIT_FAILURE);
	}
	if ((chdir("/")) < 0)
	{
		perror("Error setting root directory");
		exit(EXIT_FAILURE);
	}
	
	umask(0);
	close(STDERR_FILENO);
	// End Daemon initialization
*/
	pid = fork();
	if (pid < 0)
	{
		perror("Error forking a new process.");
		exit(EXIT_FAILURE);
	}
	if (pid > 0)
		exit(EXIT_SUCCESS);
	if ((sid = setsid()) < 0)
	{
		perror("Error running setsid");
		exit(EXIT_FAILURE);
	}
	if ((chdir("/")) < 0)
	{
		perror("Error setting root directory");
		exit(EXIT_FAILURE);
	}
	
	umask(0);
	close(STDERR_FILENO);
	close (STDIN_FILENO);
	close (STDOUT_FILENO);	
	// Initialize UPnP SDK on the internal Interface
	syslog(LOG_DEBUG, "Initializing UPnP SDK ... ");
	if ( (ret = UpnpInit(intIpAddress,0) ) != UPNP_E_SUCCESS)
	{
		syslog (LOG_ERR, "Error Initializing UPnP SDK on IP %s ",intIpAddress);
		syslog (LOG_ERR, "  UpnpInit returned %d", ret);
		UpnpFinish();
		exit(1);
	}
	syslog(LOG_DEBUG, "UPnP SDK Successfully Initialized.");

	// Set the Device Web Server Base Directory
	syslog(LOG_DEBUG, "Setting the Web Server Root Directory ... ");
	if ( (ret = UpnpSetWebServerRootDir(configDir)) != UPNP_E_SUCCESS )
	{
		syslog (LOG_ERR, "Error Setting Web Server Root Directory to: %s", configDir);
		syslog (LOG_ERR, "  UpnpSetWebServerRootDir returned %d", ret); 
		UpnpFinish();
		exit(1);
	}
	syslog(LOG_DEBUG, "Succesfully set the Web Server Root Directory.");


	// Form the Description Doc URL to pass to RegisterRootDevice
	sprintf(descDocUrl, "http://%s:%d/%s", UpnpGetServerIpAddress(),
				UpnpGetServerPort(), descDoc);

	// Register our IGD as a valid UPnP Root device
	syslog(LOG_DEBUG, "Registering the root device with descDocUrl %s", descDocUrl);
	if ( (ret = UpnpRegisterRootDevice(descDocUrl, EventHandler, &deviceHandle,
													&deviceHandle)) != UPNP_E_SUCCESS )
	{
		syslog(LOG_ERR, "Error registering the root device with descDocUrl: %s", descDocUrl);
		syslog(LOG_ERR, "  UpnpRegisterRootDevice returned %d", ret);
		UpnpFinish();
		exit(1);
	}
	syslog (LOG_DEBUG, "IGD root device successfully registered.");
	
	// Initialize the state variable table.
	StateTableInit(descDocUrl);
	
	// Record the startup time, for uptime
	startup_time = time(NULL);
	
	// Send out initial advertisements of our device's services with timeouts of 30 minutes
	if ( (ret = UpnpSendAdvertisement(deviceHandle, 1800) != UPNP_E_SUCCESS ))
	{
		syslog(LOG_ERR, "Error Sending Advertisements.  Exiting ...");
		UpnpFinish();
		exit(1);
	}
	syslog(LOG_DEBUG, "Advertisements Sent.  Listening for requests ... ");
	startup_time = time(NULL);
	// Loop until program exit signals recieved
	sigemptyset(&sigsToCatch);
	sigaddset(&sigsToCatch, SIGINT);
	sigaddset(&sigsToCatch, SIGTERM);
	//sigwait(&sigsToCatch, &signal);
	pthread_sigmask(SIG_SETMASK, &sigsToCatch, NULL);
	sigwait(&sigsToCatch, &signal);
	syslog(LOG_DEBUG, "Shutting down on signal %d...\n", signal);

	// Cleanup UPnP SDK and free memory
	pmlist_FreeList(); 

	UpnpUnRegisterRootDevice(deviceHandle);
	UpnpFinish();

	// Exit normally
	return (1);
}
