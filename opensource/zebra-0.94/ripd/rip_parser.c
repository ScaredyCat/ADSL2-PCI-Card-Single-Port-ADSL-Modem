#ifdef VTY_REMOVE
#include "rip_read_config.h"

#define CONFIG_MODE		 	0
#define INTERFACE_MODE 		1
#define RIP_MODE 			2

#ifdef DEBUG_RIPD_STANDALONE	
#define CONFIG_CMDS_LEN 		5
#else
#define CONFIG_CMDS_LEN			3
#endif

#define INTERFACE_CMDS_LEN  	10
#define RIP_CMDS_LEN			10

typedef struct command_element_t  {
		char *cmd_string;
		int (*func) (struct interface *,int, char **);
}command_element;

int command_mode = CONFIG_MODE;
struct interface *cmd_current_interface = NULL;

char cmd_type[2][12] = {
				{"interface"},
				{"router rip"}
			 };
command_element		cmd_config[] = { 
				#ifdef DEBUG_RIPD_STANDALONE	
				{"debug rip events",debug_rip_events},
				{"debug rip packet",debug_rip_packet},
				#endif
				{"log stdout",config_log_stdout},
				{"log file",config_log_file},
				{"log syslog",config_log_syslog}
		  	 }; 
command_element		cmd_interface[] = { 
	            {"interface",rip_interface},
				{"ip rip send version 1 2",ip_rip_send_version_1 },
				{"ip rip send version 2 1",ip_rip_send_version_2},
				{"ip rip send version",ip_rip_send_version},
				{"ip rip receive version 1 2",ip_rip_receive_version_1},
				{"ip rip receive version 2 1",ip_rip_receive_version_2},
				{"ip rip receive version",ip_rip_receive_version},
				{"ip rip authentication mode",ip_rip_authentication_mode},
				{"ip rip authentication key-chain",ip_rip_authentication_key_chain},
				{"ip rip authentication string",ip_rip_authentication_string }
					 				
			 };	

command_element		cmd_rip[] = {
				{"network", rip_network},
				{"neighbor",rip_neighbor},
				{"default-information originate",rip_default_information_originate},
				{"version",rip_version},
				{"passive-interface",rip_passive_interface},
				{"default-metric",rip_default_metric},
				{"timers basic",rip_timers},
				{"route",rip_route},
				{"distance",rip_distance},
				{"ip split-horizon",rip_split_horizon}
							
			 };
char* remove_spaces(char *cp) {

    if (cp == NULL) {
	    return NULL;
	}
    while (( isspace ((int) *cp) || *cp =='\r' || *cp == '\t') 
					&& *cp != '\0') {
    	cp++;
	}
	 /* Return if there is only white spaces */
	if (*cp == '\0') {
    	return NULL;
	}

	if (*cp == '!' || *cp == '#') {
    	return NULL;
	}

	while( ( isspace(cp[strlen(cp)-1]) ) || 
			( cp[strlen(cp)-1] == '\n' ) ||
			( cp[strlen(cp)-1] == '\r' ) ) {
		cp[strlen(cp)-1] = '\0';
	}

	return cp;
	
}
int execute_cmd(char *buf,command_element *cmd ) {
   	
   	int argc =0;
   	char *argv[CMD_ARGC_MAX];
   	char *temp;
   	int i =0;
    
    if ( buf == NULL || cmd== NULL ) {
		return 1;
	}
		
  if ( strcmp(buf,cmd->cmd_string) ) {	
	buf = buf + strlen(cmd->cmd_string)+1;
	strcpy(buf,remove_spaces(buf) );

   	if ( (temp = strtok(buf," ") ) ) {
		if ( !temp  ) {
			argc =0;
			argv[i] = NULL;
			goto execute;
		}
		argv[i]= malloc(strlen(temp)+1);
		if ( !argv[i] ) return -1;
		strcpy(argv[i],temp);
		argc++;
   	}
	while( ( temp = strtok(NULL," ") ) ) {
		i++;
		argv[i]= malloc(strlen(temp)+1);
		if ( !argv[i] ) return -1;
		strcpy(argv[i],temp);
		argc++;		
    }
  } else {
  	argc = 0;
	argv[0]=NULL;
  }	  
execute: 
	      	  if ( cmd->func )
		          (cmd->func)(cmd_current_interface,argc,argv);	

   	for ( i =0;i<argc;i++) {
      if (argv[i] ) {
		free(argv[i]);	
	  }
	}

	return 0;
}


void set_command_mode(char *buf) {
   if (strstr(buf,cmd_type[0])== buf ){
		command_mode = INTERFACE_MODE;
        // execute interface <> here
		execute_cmd(buf,&cmd_interface[0]);
		buf = buf + strlen(cmd_interface[0].cmd_string)+1;
		#if 1   //chandrav
	   cmd_current_interface = NULL;
       cmd_current_interface= if_lookup_by_name (buf);
	   if (!cmd_current_interface ) {
	   		fprintf (stderr, "Error occured while getting interfce:%s",buf);
		   	exit(1);
       }
		#endif

	
	}else if (strstr(buf,cmd_type[1]) == buf ) {
		command_mode = RIP_MODE;
	
	} else {
		command_mode = CONFIG_MODE;
	
	}
    
	
}

void rip_read_config_file(FILE *confp) {
   char buf[VTY_BUFSIZ+1];

   int i =0;
   int initialize_mode = 1;

   while(fgets(buf,VTY_BUFSIZ,confp) ) {
	    
	    if (remove_spaces(buf)) {
			strcpy(buf,remove_spaces(buf) );
		} else {
			continue;
		}
		
        if (initialize_mode) {
			set_command_mode(buf);
			initialize_mode = 0;
			if ( command_mode != CONFIG_MODE ) {
				continue;
			}
		}
		switch(command_mode) {
			case CONFIG_MODE:
				config_mode:
				for (i = 0; i < CONFIG_CMDS_LEN;i++) {
					if (strstr(buf,cmd_config[i].cmd_string) == buf) {
						execute_cmd(buf,&cmd_config[i]);
						break;
					}
				}
				if ( i == CONFIG_CMDS_LEN ) {
					set_command_mode(buf);
				    if ( command_mode != CONFIG_MODE ) {
						continue;
					}	
					fprintf (stderr, "Error occured during reading below line.\n%s\n",buf);
					exit(1);
				}
				break;
					
			case INTERFACE_MODE:
				for (i = 1; i < INTERFACE_CMDS_LEN;i++) {
					if (strstr(buf,cmd_interface[i].cmd_string) == buf) {
						execute_cmd(buf,&cmd_interface[i]);
						break;
					}
				}
				if ( i == INTERFACE_CMDS_LEN ) {
					set_command_mode(buf);
				    if ( command_mode != CONFIG_MODE ) {
						continue;
					} 
					goto config_mode;
					
				}
				break;

			case RIP_MODE:	
				for (i = 0; i < RIP_CMDS_LEN;i++) {
					if (strstr(buf,cmd_rip[i].cmd_string) == buf) {
						execute_cmd(buf,&cmd_rip[i]);
						break;
					}
				}
				if ( i == RIP_CMDS_LEN ) {
					set_command_mode(buf);
				    if ( command_mode == INTERFACE_MODE ) {
						continue;
					}	
					goto config_mode;
				}
				break;

		}
   }

}

#endif   /* VTY_REMOVE */
