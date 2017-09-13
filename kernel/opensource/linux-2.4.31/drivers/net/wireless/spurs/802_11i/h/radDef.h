#ifndef RADDEF_H
#define RADDEF_H

//Return Value Definition.
#ifndef SUCCESSFUL
#define SUCCESSFUL                          0
#endif
#ifndef FAILED
#define FAILED                              -1
#endif

//Port Definition.
#define RADIUSAUTHPORT                      1812
#define RADIUSACCPORT                       1813

//Number Definition.
#define NUMOFSERVER                         4
#define MAXUSERREQUEST                      50
#define MAXRETXCOUNT                        2
#define MAXWAITSECOND                       5

//Server Type.
#define SRVFORAUTH                          1
#define SRVFORACC                           2
#define SRVFORAUTHACC                       3

//Protocol Types.
#define RADCHAP                             1
#define RADPAP                              2
#define RADOTHERS                           3
#define RADMSCHAP                           4
#define RADMSCHAPCPW1                       5
#define RADMSCHAPCPW2                       6
#define RADEAP                              7
#define RADIAPP                             8

//Radius Packet Type.
#define ACCESSREQUEST                       1
#define ACCESSACCEPT                        2
#define ACCESSREJECT                        3
#define ACCOUNTINGREQUEST                   4
#define ACCOUNTINGRESPONSE                  5
#define ACCESSCHALLENGE                     11

//Packet Offsets.
#define PKTTYPE                             0
#define PKTIDENTIFIER                       1
#define PKTLENGTH                           2
#define PKTAUTHENTICATOR                    4
#define PKTATTRIBUTE                        20

//NAS Port types for attribute ANASPORTTYPE.
#define NASASYNC                            0
#define NASSYNC                             1
#define NASISDNSYNC                         2
#define NASISDNASYNCV120                    3
#define NASISDNASYNCV110                    4
#define NASVIRTUAL                          5
#define NASETHERNET                         15
#define NASWIRELESS80211                    19

//Service type
#define STCALLCHECK                         10

//Accounting status type for attribute AACCSTATUSTYPE.
#define ACCSTART                            1
#define ACCSTOP                             2
#define ACCON                               7
#define ACCOFF                              8

//Accounting Authentication type for attribute AACCAUTHENTIC.
#define AUTHRADIUS                          1
#define AUTHLOCAL                           2
#define AUTHREMOTE                          3

//Length Definition.
#define LENOFUSERNAME                       64
#define LENOFCHAPCHALLENGE                  64
#define LENOFCHAPRESPONSE                   16
#define LENOFTXPKT                          2000
#define LENOFRXPKT                          1500
#define LENOFMINPKT                         20
#define LENOFAUTHENTICATOR                  16
#define LENOFPWD                            128
#define LENOFPWDPAD                         16
#define LENOFDIGEST                         16
#define LENOFSECRET                         34 //max size 32, for DWORD struct
#define LENOFNASID                          64
#define LENOFSERVICESTRING                  256
#define LENOFSTAID                          6
#define LENOFNUMATTR                        6
#define LENOFSESSIONID                      50

//RADIUS Packet Attribute Types.
#define ANOATTRIBUTE                        -1
#define AUSERNAME                           1
#define AUSERPASSWORD                       2
#define ACHAPPASSWORD                       3
#define ANASIPADDRESS                       4
#define ANASPORT                            5
#define ASERVICETYPE                        6
#define AFRAMEDPROTOCOL                     7
#define AFRAMEDIPADDRESS                    8
#define AFRAMEDIPNETMASK                    9
#define AFRAMEDROUTING                      10
#define AFILTERID                           11
#define AFRAMEDMTU                          12
#define AFRAMEDCOMPRESSION                  13
#define ALOGINIPHOST                        14
#define ALOGINSERVICE                       15
#define ALOGINTCPPORT                       16
#define AREPLYMESSAGE                       18
#define ACALLBACKNUMBER                     19
#define ACALLBACKID                         20
#define AFRAMEDROUTE                        22
#define ASTATE                              24
#define ACLASS                              25
#define AVENDORSPECIFIC                     26
#define ACALLEDSTATIONID                    30
#define ACALLINGSTATIONID                   31
#define ANASIDENTIFIER                      32
#define APROXYSTATE                         33
#define ALOGINLATSERVICE                    34
#define ALOGINLATNODE                       35
#define AFRAMEDAPPLETALKZONE                39
#define AACCSTATUSTYPE                      40
#define AACCINPUTOCTETS                     42
#define AACCOUTPUTOCTETS                    43
#define AACCSESSIONID                       44
#define AACCAUTHENTIC                       45
#define AACCSESSIONTIME                     46
#define AACCINPUTPACKETS                    47
#define AACCOUTPUTPACKETS                   48
#define AACCTERMINATECAUSE                  49
#define ACHAPCHALLENGE                      60
#define ANASPORTTYPE                        61
#define ALOGINLATPORT                       63
#define AEAPMESSAGE                         79
#define AMESSAGEAUTHENTICATOR               80
#define ATUNNELSERVERAUTHID                 91

//Unused Attributes.==========================
//Attribute Type 17 unassigned
//Attribute Type 21 unassigned
#define AFRAMEDIPXNETWORK                   23
#define ASESSIONTIMEOUT                     27
#define AIDLETIMEOUT                        28
#define ATERMINATIONACTION                  29
#define ALOGINLATGROUP                      36
#define AFRAMEDAPPLETALKLINK                37
#define AFRAMEDAPPLETALKNETWORK             38
#define AACCDELAYTIME                       41
#define AACCMULTISESSIONID                  50
#define AACCLINKCOUNT                       51
#define AACCINPUTGIGAWORDS                  52
#define AACCOUTPUTGIGAWORDS                 53
//Attribute Type 54 Unused.
#define AEVENTTIMESTAMP                     55
#define APORTLIMIT                          62
#define ATUNNELTYPE                         64
#define ATUNNELMEDIUMTYPE                   65
#define ATUNNELCLIENTENDPOINT               66
#define ATUNNELSERVERENDPOINT               67
#define ATUNNELCONNECTION                   68
#define ATUNNELPASSWORD                     69
#define AARAPPASSWORD                       70
#define AARAPFEATURES                       71
#define AARAPZONEACCESS                     72
#define AARAPSECURITY                       73
#define AARAPSECURITYDATA                   74
#define APASSWORDRETRY                      75
#define APROMPT                             76
#define ACONNECTINFO                        77
#define ACONFIGURATIONTOKEN                 78
#define ATUNNELPRIVATEGROUPID               81
#define ATUNNELASSIGNMENTID                 82
#define ATUNNELPREFERENCE                   83
#define AARAPCHALLENGERESPONSE              84
#define AACCINTERIMINTERVAL                 85
#define ANASPORTID                          87
#define AFRAMEDPOOL                         88
#define ATUNNELCLIENTAUTHID                 90

//============================================
//Radius Attribute length.
#define LENOFCHAPPASSWORD                   19  //Attribute Type 3.
#define LENOFNASIPADDRESS                   6   //Attribute Type 4.
#define LENOFNASPORT                        6   //Attribute Type 5.
#define LENOFSERVICETYPE                    6   //Attribute Type 6.
#define LENOFFRAMEDPROTOCOL                 6   //Attribute Type 7.
#define LENOFFRAMEDIPADDRESS                6   //Attribute Type 8.
#define LENOFFRAMEDIPNETMASK                6   //Attribute Type 9.
#define LENOFFRAMEDROUTING                  6   //Attribute Type 10.
#define LENOFFRAMEDMTU                      6   //Attribute Type 12.
#define LENOFFRAMEDCOMPRESSION              6   //Attribute Type 13.
#define LENOFLOGINIPHOST                    6   //Attribute Type 14.
#define LENOFLOGINSERVICE                   6   //Attribute Type 15.
#define LENOFLOGINTCPPORT                   6   //Attribute Type 16.
#define LENOFFRAMEDIPXNETWORK               6   //Attribute Type 23.
#define LENOFSESSIONTIMEOUT                 6   //Attribute Type 27.
#define LENOFIDLETIMEOUT                    6   //Attribute Type 28.
#define LENOFTERMINATIONACTION              6   //Attribute Type 29.
#define LENOFLOGINLATGROUP                  34  //Attribute Type 36.
#define LENOFFRAMEDAPPLETALKLINK            6   //Attribute Type 37.
#define LENOFFRAMEDAPPLETALKNETWORK         6   //Attribute Type 38.
#define LENOFACCSTATUSTYPE                  6   //Attribute Type 40.
#define LENOFACCDELAYTIME                   6   //Attribute Type 41.
#define LENOFACCINPUTOCTETS                 6   //Attribute Type 42.
#define LENOFACCOUTPUTOCTETS                6   //Attribute Type 43.
#define LENOFACCAUTHENTIC                   6   //Attribute Type 45.
#define LENOFACCSESSIONTIME                 6   //Attribute Type 46.
#define LENOFACCINPUTPACKETS                6   //Attribute Type 47.
#define LENOFACCOUTPUTPACKETS               6   //Attribute Type 48.
#define LENOFACCTERMINATECAUSE              6   //Attribute Type 49.
#define LENOFACCLINKCOUNT                   6   //Attribute Type 51.
#define LENOFACCTINPUTGIGAWORDS             6   //Attribute Type 52.
#define LENOFACCTOUTPUTGIGAWORDS            6   //Attribute Type 53.
#define LENOFEVENTTIMESTAMP                 6   //Attribute Type 55.
#define LENOFNASPORTTYPE                    6   //Attribute Type 61.
#define LENOFPORTLIMIT                      6   //Attribute Type 62.
#define LENOFTUNNELTYPE                     6   //Attribute Type 64.
#define LENOFTUNNELMEDIUMTYPE               6   //Attribute Type 65.
#define LENOFARAPPASSWORD                   18  //Attribute Type 70.
#define LENOFARAPFEATURES                   16  //Attribute Type 71.
#define LENOFARAPZONEACCESS                 6   //Attribute Type 72.
#define LENOFARAPSECURITY                   6   //Attribute Type 73.
#define LENOFPASSWORDRETRY                  6   //Attribute Type 75.
#define LENOFPROMPT                         6   //Attribute Type 76.
#define LENOFMESSAGEAUTHENTICATOR           18  //Attribute Type 80.
#define LENOFTUNNELPERFERENCE               6   //Attribute Type 83.
#define LENOFARAPCHALLENGERESPONSE          10  //Attribute Type 84.
#define LENOFACCINTERIMINTERVAL             6   //Attribute Type 85.
#define LENOFACCTUNNELPACKETSLOST           6   //Attribute Type 86.

//MSCHAP & MSCHAP_CPW1 & MSCHAP_CPW2 Definition.
#define MSCHAPVENDORID                      311
#define MSCHAPRESPONSEFLAG                  0
#define MSCHAPRESPONSE                      1
#define MSCHAPERROR                         2
#define MSCHAPCPW1                          3
#define MSCHAPCPW2                          4
#define MSCHAPLMENCPW                       5
#define MSCHAPNTENCPW                       6
#define MSCHAPDOMAIN                        10
#define MSCHAPCHALLENGE                     11
#define MSCHAPMPPEKEYS                      12

//Length Definition of MSCHAP.
#define LENOFMSCHAPCPW1                     72
#define LENOFMSCHAPCPW2                     86
#define NUMOFFRAGSMSCHAPENCPW               3
//#define   LENOFMSCHAPMPPEKEYS             34
#define LENOFMSCHAPLMRESPONSE               24
#define LENOFMSCHAPNTRESPONSE               24
#define LENOFMSCHAPRESPONSE                 52
#define LENOFMSCHAPCHALLENGE                64
#define LENOFMSCHAPLMOLDPWD                 16
#define LENOFMSCHAPNTOLDPWD                 16
#define LENOFMSCHAPLMNEWPWD                 16
#define LENOFMSCHAPNTNEWPWD                 16
#define LENOFMSCHAPOLDLMHASH                16
#define LENOFMSCHAPOLDNTHASH                16
#define LENOFMSCHAPLMENCPWD                 516
#define LENOFMSCHAPNTENCPWD                 516
#define LENOFMSCHAPENCPWFRAG                172 //(172*3=516).

//MSCHAP attribue flags and codes.
#define MSCHAPCPW1CODE                      5
#define MSCHAPCPW2CODE                      6
#define MSCHAPLMENCPWCODE                   6
#define MSCHAPNTENCPWCODE                   6
#define MSCHAPRESPONSEFLAG                  0
#define MSCHAPCPW1FLAG                      0
#define MSCHAPCPW2FLAG                      0

#define RADFRAMEMTU                         1400
#define RADNASPORT                          37


#endif

