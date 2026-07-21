char *los_error_code(signed long loserror)
{

    switch (loserror)
    {
    case -6081:
        return "End of exec file input";
    case -6004:
        return "Attempt to reset text file with typed-file type";
    case -6003:
        return "Attempt to reset nontext file with text type";
    case -1885:
        return "ProFile not present during driver initialization";
    case -1882:
        return "ProFile not present during driver initialization";
    case -1176:
        return "Data in the object have been altered by Scavenger";
    case -1175:
        return "File or volume was scavenged";
    case -1174:
        return "File was left open or volume was left mounted, and system crashed";
    case -1173:
        return "File was last closed by the OS";
    case -1146:
        return "Only a portion of the space requested was allocated";
    case -1063:
        return "Attempt to mount boot volume from another Lisa or not most recent boot volume";
    case -1060:
        return "Attempt to mount a foreign boot disk following a temporary unmount";
    case -1059:
        return "The bad block directory of the diskette is almost full or difficult to read";
    case -876:
        return "(from CLOSE_OBJECT) The file just closed. May be damaged or contain corrupted data because an I/O error occurred when writing a part of the file that was buffered by the file system";
    case -696:
        return "Printer out of paper during initialization";
    case -660:
        return "Cable disconnected during ProFile initialization";
    case -626:
        return "Scavenger indicated data are questionable, but may be OK";
    case -622:
        return "Parameter memory and the disk copy were both invalid";
    case -621:
        return "Parameter memory was invalid but the disk copy was valid";
    case -620:
        return "Parameter memory was valid but the disk copy was invalid";
    case -413:
        return "Event channel was scavenged";
    case -412:
        return "Event channel was left open and system crashed";
    case -321:
        return "Data segment open when the system crashed. Data possibly invalid.";
    case -320:
        return "Could not determine size of data segment";
    case -150:
        return "Process was created, but a library used by program has been scavenged and altered";
    case -149:
        return "Process was created, but the specified program file has been scavenged and altered";
    case -125:
        return "Specified process is already terminating";
    case -120:
        return "Specified process is already active";
    case -115:
        return "Specified process is already suspended";
    case 100:
        return "Specified process does not exist";
    case 101:
        return "Specified process is a system process";
    case 110:
        return "Invalid priority specified (must be l..225)";
    case 130:
        return "Could not open program file";
    case 131:
        return "File System error while trying to read program file";
    case 132:
        return "Invalid program file (incorrect format)";
    case 133:
        return "Could not get a stack segment for new process";
    case 134:
        return "Could not get a syslocal segment for new process";
    case 135:
        return "Could not get sysglobal space for new process";
    case 136:
        return "Could not set up communication channel for new process";
    case 138:
        return "Error accessing program file while loading";
    case 141:
        return "Error accessing a library file while loading program";
    case 142:
        return "Cannot run protected file on this machine";
    case 143:
        return "Program uses an intrinsic unit not found in the Intrinsic Library";
    case 144:
        return "Program uses an intrinsic unit whose name/type does not agree with";
    case the:
        return "Intrinsic Library";
    case 145:
        return "Program uses a shared segment not found in the Intrinsic Library";
    case 146:
        return "Program uses a shared segment whose name does not agree with the Intrinsic Library";
    case 147:
        return "No space in syslocal for program file descriptor during process creation";
    case 148:
        return "No space in the shared IU data segment for the program's shared IU globals";
    case 190:
        return "No space in syslocal for program tile description during List_LibFiles operation";
    case 191:
        return "Could not open program file";
    case 192:
        return "Error trying to read program file";
    case 193:
        return "Cannot read protected program file";
    case 194:
        return "Invalid program file (incorrect format)";
    case 195:
        return "Program uses a shared segment not found in the Intrinsic Library";
    case 196:
        return "Program uses a shared segment whose name does not agree with the Intrinsic Library";
    case 198:
        return "Disk I/O error trying to read the intrinsic unit directory";
    case 199:
        return "Specified library file number does not exist in the Intrinsic Library";
    case 201:
        return "No such exception name declared";
    case 202:
        return "No space left in the system data area for Declare_Excep_Hdl or Signal_Excep";
    case 203:
        return "Null name specified as exception name";
    case 302:
        return "Invalid LDSN";
    case 303:
        return "No data segment hound to the LDSN";
    case 304:
        return "Data segment already bound to the LDSN";
    case 306:
        return "Data segment too large";
    case 307:
        return "Input data segment path name is invalid";
    case 308:
        return "Data segment already exists";
    case 309:
        return "Insufficient disk space for data segment";
    case 310:
        return "An invalid size has been specified";
    case 311:
        return "Insufficient system resources";
    case 312:
        return "Unexpected File System error";
    case 313:
        return "Data segment not found";
    case 314:
        return "Invalid address passed to Info_Address";
    case 315:
        return "Insufficient memory for operation";
    case 317:
        return "Disk error while trying to swap in data segment";
    case 401:
        return "Invalid event channel name passed to Make_Event_Chn";
    case 402:
        return "No space left in system global data area for Open_Event_Chn";
    case 403:
        return "No space left in system local data area for Open_Event_Chn";
    case 404:
        return "Non-block-structured device specified in pathname";
    case 405:
        return "Catalog is full in Make_Event_Chn or Open Event_Chn";
    case 406:
        return "No such event channel exists in Kill_Event_Chn";
    case 410:
        return "Attempt to open a local event channel to send";
    case 411:
        return "Attempt to open event channel to receive when event channel has a receiver";
    case 413:
        return "Unexpected File System error in Open_Event_Chn";
    case 416:
        return "Cannot get enough disk space for event channel in Open_Event_Chn";
    case 417:
        return "Unexpected File System error in Close_Event_Chn";
    case 420:
        return "Attempt to wait on a channel that the calling process did not open";
    case 421:
        return "Wait_Event_Chn returns empty because sender process could not complete";
    case 422:
        return "Attempt to call Wait_Event_Chn on an empty event-call channel";
    case 423:
        return "Cannot find corresponding event channel after being blocked";
    case 424:
        return "Amount of data returned while reading from event channel not of expected size";
    case 425:
        return "Event channel empty after being unblocked, Wait_Event_Chn";
    case 426:
        return "Bad request pointer error returned in Wait_Event_Chn";
    case 427:
        return "Wait List has illegal length specified";
    case 428:
        return "Receiver unblocked because last sender closed";
    case 429:
        return "Unexpected File System error in Wait_Event_Chn";
    case 430:
        return "Attempt to send to a channel which the calling process does not have open";
    case 431:
        return "Amount of data transferred while writing to event channel not of expected size";
    case 432:
        return "Sender unblocked because receiver closed in Send_Event_Chn";
    case 433:
        return "Unexpected File System error in Send_Event_Chn";
    case 440:
        return "unexpected File System error in Make_Event_Chn";
    case 441:
        return "Event channel already exists in Make_Event_Chn";
    case 445:
        return "Unexpected File System error in Kill_Event_Chn";
    case 450:
        return "Unexpected File System error in Flush Event_Chn";
    case 530:
        return "Size of stack expansion request exceeds limit specified for program";
    case 531:
        return "Cannot perform explicit stack expansion due to lack of memory";
    case 532:
        return "Insufficient disk space for explicit stack expansion";
    case 600:
        return "Attempt to perform I/O operation on non I/O request";
    case 602:
        return "No more alarms available during driver initialization";
    case 605:
        return "Call to nonconfigured device driver";
    case 606:
        return "Cannot find sector on floppy diskette (disk unformatted)";
    case 608:
        return "Illegal length or disk address for transfer";
    case 609:
        return "Call to nonconfigured device driver";
    case 610:
        return "No more room in sysglobal for I/O request";
    case 613:
        return "Unpermitted direct access to spare track with sparing enabled on floppy drive";
    case 614:
        return "No disk present in drive";
    case 615:
        return "Wrong call version to floppy drive";
    case 616:
        return "Unpermitted floppy drive function";
    case 617:
        return "Checksum error on floppy diskette";
    case 618:
        return "Cannot format, or write protected, or error unclamping floppy diskette";
    case 619:
        return "No more room in sysglobal for I/O request";
    case 623:
        return "Illegal device control parameters to floppy drive";
    case 625:
        return "Scavenger indicated data are bad";
    case 630:
        return "The time passed to Delay_Time, Convert_Time, or Send_Event_Chn has invalid year";
    case 631:
        return "Illegal timeout request parameter";
    case 632:
        return "No memory available to initialize clock";
    case 634:
        return "Illegal timed event id of [DEL: 1 :DEL]";
    case 635:
        return "Process got unblocked prematurely due to process termination";
    case 636:
        return "Timer request did not complete successfully";
    case 638:
        return "Time passed to Delay_Time or Send_Event_Chn more than 23 days from current time";
    case 639:
        return "Illegal date passed to Set_Time, or illegal date from system clock in Get Time";
    case 640:
        return "RS 232 driver called with wrong version number";
    case 641:
        return "RS-232 read or write initiated with illegal parameter";
    case 642:
        return "Unimplemented or unsupported RS-232 driver function";
    case 646:
        return "No memory available to initialize RS-232";
    case 647:
        return "Unexpected RS-232 timer interrupt";
    case 648:
        return "Unpermitted RS-232 initialization, or disconnect detected";
    case 649:
        return "Illegal device control parameters to RS-232";
    case 652:
        return "N-port driver not initialized prior to ProFile";
    case 653:
        return "No room in sysglobal to initialize ProFile";
    case 654:
        return "Hard error status returned from drive";
    case 655:
        return "Wrong call version to ProFile";
    case 656:
        return "Unpermitted ProFile function";
    case 657:
        return "Illegal device control parameter to ProFile";
    case 658:
        return "Premature end of file when reading from driver";
    case 659:
        return "Corrupt File System header chain found in driver";
    case 660:
        return "Cable disconnected";
    case 662:
        return "Parity error while sending command or writing data to ProFile";
    case 663:
        return "Checksum error or CRC error or parity error in data read";
    case 666:
        return "Timeout";
    case 670:
        return "Bad command response from drive";
    case 671:
        return "Illegal length specified (must = 1 on input)";
    case 672:
        return "Unimplemented console driver function";
    case 673:
        return "No memory available to initialize console";
    case 674:
        return "Console driver called with wrong version number";
    case 675:
        return "Illegal device control";
    case 680:
        return "Wrong call version to serial driver";
    case 682:
        return "Unpermitted serial driver function";
    case 683:
        return "No room in sysglobal to initialize serial driver";
    case 685:
        return "Eject not allowed for this device";
    case 686:
        return "No room in sysglobal to initialize n-port card driver";
    case 687:
        return "Unpermitted n-port card driver function";
    case 688:
        return "Wrong call version to n-port card driver";
    case 690:
        return "Wrong call version to parallel printer";
    case 691:
        return "Illegal parallel printer parameters";
    case 692:
        return "N-port card not initialized prior to parallel printer";
    case 693:
        return "No room in sysgloball. to initialize parallel printer";
    case 694:
        return "Unimplemented parallel printer function";
    case 695:
        return "Illegal device control parameters (parallel printer)";
    case 696:
        return "Printer out of paper";
    case 698:
        return "Printer offline";
    case 699:
        return "No response from printer";
    case 700:
        return "Mismatch between loader version number and Operating System version number";
    case 701:
        return "OS exhausted its internal space during startup";
    case 702:
        return "Cannot make system process";
    case 703:
        return "Cannot kill pseudo-outer process";
    case 704:
        return "Cannot create driver";
    case 706:
        return "Cannot initialize floppy disk driver";
    case 707:
        return "Cannot initialize the File System volume";
    case 708:
        return "Hard disk mount table unreadable";
    case 709:
        return "Cannot map screen data";
    case 710:
        return "Too many slot-based devices";
    case 724:
        return "The boot tracks do not know the right File System version";
    case 725:
        return "Either damaged File System or damaged contents";
    case 726:
        return "Boot device read failed";
    case 727:
        return "The OS will not fit into the available memory";
    case 728:
        return "SYSTEM.OS is missing";
    case 729:
        return "SYSTEM.CONFIG is corrupt";
    case 730:
        return "SYSTEM.OS is corrupt";
    case 731:
        return "SYSTEM.DEBUG or SYSTEM.DEBUG2 is corrupt";
    case 732:
        return "SYSTEM.LLD is corrupt";
    case 733:
        return "Loader range error";
    case 734:
        return "Wrong driver is found. For instance, storing a diskette loader on a ProFile";
    case 735:
        return "SYSTEM.LLD is missing";
    case 736:
        return "SYSTEM.UNPACK is missing";
    case 737:
        return "Unpack of SYSTEM.OS with SYSTEM.UNPACK failed";
    case 791:
        return "Configurable Driver (CD) code file not executable";
    case 792:
        return "Could not get code space for the CD";
    case 793:
        return "I/O error reading CD code file";
    case 794:
        return "CD code file not found";
    case 795:
        return "CD has more than 1 segment";
    case 796:
        return "Could not get temporary space while loading CD";
    case 801:
        return "I/O Result <> 0 on I/O using the Monitor";
    case 802:
        return "Asynchronous I/O request not completed successfully";
    case 803:
        return "Bad combination of mode parameters";
    case 806:
        return "Page specified is out of range";
    case 809:
        return "Invalid arguments (page, address, offset, or count)";
    case 810:
        return "The requested page could not be read in";
    case 816:
        return "Not enough sysglobal space for File System buffers";
    case 819:
        return "Bad device number";
    case 820:
        return "No space in sysglobal for asynchronous request list";
    case 821:
        return "Already initialized I/O for this device";
    case 822:
        return "Bad device number";
    case 825:
        return "Error in parameter values (Allocate)";
    case 826:
        return "No more room to allocate pages on device";
    case 828:
        return "Error in parameter values (Deallocate)";
    case 829:
        return "Partial deallocation only (ran into unallocated region)";
    case 835:
        return "Invalid s-file number";
    case 837:
        return "Unallocated s-file or I/O error";
    case 838:
        return "Map overflow: s-file too large";
    case 839:
        return "Attempt to compact file past PEOF";
    case 840:
        return "The allocation map of this file is truncated";
    case 841:
        return "Unallocated s-file or I/O error";
    case 843:
        return "Requested exact fit, but one could not be provided";
    case 847:
        return "Requested transfer count is <= 0";
    case 848:
        return "End of file encountered";
    case 849:
        return "Invalid page or offset value in parameter list";
    case 852:
        return "Bad unit number";
    case 854:
        return "No free slots in s-list directory (too many s-files)";
    case 855:
        return "No available disk space for file hints";
    case 856:
        return "Device not mounted";
    case 857:
        return "Empty, locked, or invalid s-file";
    case 861:
        return "Relative page is beyond PEOF (bad parameter value)";
    case 864:
        return "No sysglobal space for volume bitmap";
    case 866:
        return "Wrong FS version or not a valid Lisa FS volume";
    case 867:
        return "Bad unit number";
    case 868:
        return "Bad unit number";
    case 869:
        return "Unit already mounted (mount)/no unit mounted";
    case 870:
        return "No sysglobal space for DCB or MDDF";
    case 871:
        return "Parameter not a valid s-file ID";
    case 872:
        return "No sysglobal space for s-file control block";
    case 873:
        return "Specified file is already open for private access";
    case 874:
        return "Device not mounted";
    case 875:
        return "Invalid s-file ID or s-file control block";
    case 879:
        return "Attempt to position past LEOF";
    case 881:
        return "Attempt to read empty file";
    case 882:
        return "No space on volume for new data page of file";
    case 883:
        return "Attempt to read past LEOF";
    case 884:
        return "Not first auto-allocation, but file was empty";
    case 885:
        return "Could not update filesize hints after a write";
    case 886:
        return "No syslocal space for I/O request list";
    case 887:
        return "Catalog pointer does not indicate a catalog (bad parameter)";
    case 888:
        return "Entry not found in catalog";
    case 890:
        return "Entry that name already exists";
    case 891:
        return "Catalog is full or is damaged";
    case 892:
        return "Illegal name for an entry";
    case 894:
        return "Entry not round, or catalog is damaged";
    case 895:
        return "Invalid entry name";
    case 896:
        return "Safety switch is on-[DEL: cannot kill entry :DEL]";
    case 897:
        return "Invalid bootdev value";
    case 899:
        return "Attempt to allocate a pipe";
    case 900:
        return "Invalid page count or FCB pointer argument";
    case 901:
        return "Could not satisfy allocation request";
    case 921:
        return "Pathname invalid or no such device";
    case 922:
        return "Invalid label size";
    case 926:
        return "Pathname invalid or no such device";
    case 927:
        return "Invalid label size";
    case 941:
        return "Pathname invalid or no such device";
    case 944:
        return "Object is not a file";
    case 945:
        return "File is not in the killed state";
    case 946:
        return "Pathname invalid or no such device";
    case 947:
        return "Not enough space in syslocal for File System retdb";
    case 948:
        return "Entry not found in specified catalog";
    case 949:
        return "Private access not allowed if file already open shared";
    case 950:
        return "Pipe already in use, requested access not possible or dwrite not allowed";
    case 951:
        return "File is already opened in private mode";
    case 952:
        return "Bad refnum";
    case 954:
        return "Bad refnum";
    case 955:
        return "Read access not allowed to specified object";
    case 956:
        return "Attempt to position FMARK past LEOF not allowed";
    case 957:
        return "Negative request count is illegal";
    case 958:
        return "Nonsequential access is not allowed";
    case 959:
        return "System resources exhausted";
    case 960:
        return "Error writing to pipe while an unsatisfied read was pending";
    case 961:
        return "Bad refnum";
    case 962:
        return "No WRITE or APPEND access allowed";
    case 963:
        return "Attempt to position FMARK too far past LEOF";
    case 964:
        return "Append access not allowed in absolute mode";
    case 965:
        return "Append access not allowed in relative mode";
    case 966:
        return "Internal inconsistency of FMARK and LEOF (warning)";
    case 967:
        return "Nonsequential access is not allowed";
    case 968:
        return "Bad refnum";
    case 971:
        return "Pathname invalid or no such device";
    case 972:
        return "Entry not found in specified catalog";
    case 974:
        return "Bad refnum";
    case 977:
        return "Bad refnum";
    case 978:
        return "Page count is nonpositive";
    case 979:
        return "Not a block structured device";
    case 981:
        return "Bad refnum";
    case 982:
        return "No space has been allocated for specified file";
    case 983:
        return "Not a block-structured device";
    case 985:
        return "Bad refnum";
    case 986:
        return "No space has been allocated for specified file";
    case 987:
        return "Not a block-structured device";
    case 988:
        return "Bad refnum";
    case 989:
        return "Caller is not a reader of the pipe";
    case 990:
        return "Not a block-structured device";
    case 994:
        return "Invalid refnum";
    case 995:
        return "Not a block-structured device";
    case 999:
        return "Asynchronous read was unblocked before it was satisfied";
    case 1021:
        return "Pathname invalid or no such entry";
    case 1022:
        return "No such entry found";
    case 1023:
        return "Invalid newname, check for [DEL: ' in string :DEL]";
    case 1024:
        return "New name already exists in catalog";
    case 1031:
        return "Pathname invalid or no such entry";
    case 1032:
        return "Invalid transfer count";
    case 1033:
        return "No such entry found";
    case 1041:
        return "Pathname invalid or no such entry";
    case 1042:
        return "Invalid transfer count";
    case 1043:
        return "No such entry found";
    case 1051:
        return "No device or volume by that name";
    case 1052:
        return "A volume is already mounted on device";
    case 1053:
        return "Attempt to mount temporarily unmounted hoot volume just unmounted from this Lisa";
    case 1054:
        return "The had block directory of the diskette is invalid";
    case 1061:
        return "No device or volume by that name";
    case 1062:
        return "No volume is mounted on device";
    case 1071:
        return "Not a valid or mounted volume for working directory";
    case 1091:
        return "Pathname invalid or no such entry";
    case 1092:
        return "No such entry found";
    case 1101:
        return "Invalid device name";
    case 1121:
        return "Invalid device, not mounted, or catalog is damaged";
    case 1122:
        return "(from RESET_CATALOG) There is no space available in the process' local data area to allocate the catalog scan buffer";
    case 1124:
        return "(from GET_NEXT ENTRY) There is no space available in the process' local data area to allocate the catalog scan buffer";
    case 1128:
        return "Invalid pathname, device, or volume not mounted";
    case 1130:
        return "File is protected; cannot open due to protection violation";
    case 1131:
        return "No device or volume by that name";
    case 1132:
        return "No volume is mounted on that device";
    case 1133:
        return "No more open files in the file list of that device";
    case 1134:
        return "Cannot find space in sysglobal for open file list";
    case 1135:
        return "Cannot find the open file entry to modify";
    case 1136:
        return "Boot volume not mounted";
    case 1137:
        return "Boot volume already unmounted";
    case 1138:
        return "Caller cannot have higher priority than system processes when calling ubd";
    case 1141:
        return "Boot volume was not unmounted when calling rbd";
    case 1142:
        return "Some other volume still mounted on the boot device when calling rbd";
    case 1143:
        return "No sysglobal space for MDDF to do rbd";
    case 1144:
        return "Attempt to remount volume which is not the temporarily unmounted hoot volume";
    case 1145:
        return "No sysglobal space for hit map to do rbd";
    case 1158:
        return "Track-by-track copy buffer is too small";
    case 1159:
        return "Shutdown requested while boot volume was unmounted";
    case 1160:
        return "Destination device too small for track-by-track copy";
    case 1161:
        return "Invalid final shutdown mode";
    case 1162:
        return "Power is already off";
    case 1163:
        return "Illegal command";
    case 1164:
        return "Device is not a diskette device";
    case 1165:
        return "No volume is mounted on the device";
    case 1166:
        return "A valid volume is already mounted on the device";
    case 1167:
        return "Not a block-structured device";
    case 1168:
        return "Device name is invalid";
    case 1169:
        return "Could not access device before initialization using default device parameters";
    case 1170:
        return "Could not mount volume after initialization";
    case 1171:
        return "'-' is not allowed in a volume name";
    case 1172:
        return "No space available to initialize a bitmap for the volume";
    case 1176:
        return "Cannot read from a pipe more than half of its allocated physical size";
    case 1177:
        return "Cannot cancel a read request for a pipe";
    case 1178:
        return "Process waiting for pipe data got unblocked because last pipe writer closed it";
    case 1180:
        return "Cannot write to a pipe more than halt of its allocated physical size";
    case 1181:
        return "No system space left for request block for pipe";
    case 1182:
        return "Writer process to a pipe got unblocked before the request was satisfied";
    case 1183:
        return "Cannot cancel a write request for a pipe";
    case 1184:
        return "Process waiting for pipe space got unblocked because the reader closed the pipe";
    case 1186:
        return "Cannot allocate space to a pipe while it has data wrapped around";
    case 1188:
        return "Cannot compact a pipe while it has data wrapped around";
    case 1190:
        return "Attempt to access a page that is not allocated to the pipe";
    case 1191:
        return "Bad parameter";
    case 1193:
        return "Premature end of file encountered";
    case 1196:
        return "Something is still open on device-[DEL: cannot unmount :DEL]";
    case 1197:
        return "Volume is not formatted or cannot be read";
    case 1198:
        return "Negative request count is illegal";
    case 1199:
        return "Function or procedure is not yet implemented";
    case 1200:
        return "Illegal volume parameter";
    case 1201:
        return "Blank file parameter";
    case 1202:
        return "Error writing destination file";
    case 1203:
        return "Invalid UCSD directory";
    case 1204:
        return "File not found";
    case 1210:
        return "Boot track program not executable";
    case 1211:
        return "Boot track program too big";
    case 1212:
        return "Error reading boot track program";
    case 1213:
        return "Error writing boot track program";
    case 1214:
        return "Boot track program file not found";
    case 1215:
        return "Cannot write boot tracks on that device";
    case 1216:
        return "Could not create/close internal buffer";
    case 1217:
        return "Boot track program has too many code segments";
    case 1218:
        return "Could not find configuration information entry";
    case 1219:
        return "Could not get enough working space";
    case 1220:
        return "Premature SOP in boot track program";
    case 1221:
        return "Position out of range";
    case 1222:
        return "No device at that position";
    case 1225:
        return "Scavenger has detected an internal inconsistency symptomatic of a software bug";
    case 1226:
        return "Invalid device name";
    case 1227:
        return "Device is not block structured";
    case 1228:
        return "Illegal attempt to scavenge the boot volume";
    case 1229:
        return "Cannot read consistently from the volume";
    case 1230:
        return "Cannot write consistently to the volume";
    case 1231:
        return "Cannot allocate space (Heap segment)";
    case 1232:
        return "Cannot allocate space (Map segment)";
    case 1233:
        return "Cannot allocate space (SFDB segment)";
    case 1237:
        return "Error rebuilding the volume root directory";
    case 1240:
        return "Illegal attempt to scavenge a non OS-formatted volume";
    case 1295:
        return "The allocation map of this file is damaged and cannot be read";
    case 1296:
        return "Bad string argument has been passed";
    case 1297:
        return "Entry name for the object is invalid (on the volume)";
    case 1298:
        return "S-list entry for the object is invalid (on the volume)";
    case 1807:
        return "No disk in floppy drive";
    case 1820:
        return "Write-protect error on floppy drive";
    case 1822:
        return "Unable to clamp floppy drive";
    case 1824:
        return "Floppy drive write error";
    case 1882:
        return "Bad response from ProFile";
    case 1885:
        return "ProFile timeout error";
    case 1998:
        return "Invalid parameter address";
    case 1999:
        return "Bad refnum";
    case 6001:
        return "Attempt to access unopened file";
    case 6002:
        return "Attempt to reopen a file which is not closed using an open FIB (file info block)";
    case 6003:
        return "Operation incompatible with access mode";
    case 6004:
        return "Printer offline";
    case 6005:
        return "File record type incompatible with character device (must be byte sized)";
    case 6006:
        return "Bad integer (read)";
    case 6010:
        return "Operation incompatible with file type or access mode";
    case 6081:
        return "Premature end of exec file";
    case 6082:
        return "Invalid exec (temporary) file name";
    case 6083:
        return "Attempt to set prefix with null name";
    case 6090:
        return "Attempt to move console with exec or output file open";
    case 6101:
        return "Bad real (read)";
    case 6151:
        return "Attempt to reinitialize heap already in use";
    case 6152:
        return "Bad argument to NEW (negative size)";
    case 5153:
        return "Insufficient memory for NEW request";
    case 6154:
        return "Attempt to RELEASE outside of heap";
    case 10050:
        return "Request block is not chained to a PCB (Unblk_Req)";
    case 10051:
        return "Bld_Req is called with interrupts off";
    case 10100:
        return "An error was returned from SetUp_Directory or a Data Segment Routine Setup_IUInfo)";
    case 10102:
        return "Error > 0 trying to create shell (Root)";
    case 10103:
        return "Sem_Count > 1 (mit_Sem)";
    case 10104:
        return "Could not open event channel for shell (Root)";
    case 10197:
        return "Automatic stack expansion fault occurred in system code (Check_Stack)";
    case 10198:
        return "Need_Mem set for current process while scheduling is disabled SimpleScheduler)";
    case 10199:
        return "Attempt to block for reason other than I/O while scheduling is disabled (SimpleScheduler)";
    case 10201:
        return "Hardware exception occurred while in system code";
    case 10202:
        return "No space left from Sigl Excep call in Hard_Excep";
    case 10203:
        return "No space left from Sigl Excep call in Nmi_Excep";
    case 10205:
        return "Error from Wait_Event_Chn called in Excep_Prolog";
    case 10207:
        return "No system data space in Excep_Setup";
    case 10208:
        return "No space left from Sigl Excep call in range error";
    case 10212:
        return "Error in Term_Def_Hdl from Enable_Excep";
    case 10213:
        return "Error in Force_Term_Excep, no space in Enq_Ex_Data";
    case 10401:
        return "Error from Close_Event Chn in Sc Cleanup";
    case 10582:
        return "Unable to get space in Freeze_Seq";
    case 10590:
        return "Fatal memory parity error";
    case 10593:
        return "Unable to move memory manager segment during startup";
    case 10594:
        return "Unable to swap in a segment during startup";
    case 10595:
        return "Unable to get space in Extend MMlist";
    case 10596:
        return "Trying to alter size of segment that is not data or stack (Alt_DS Size)";
    case 10597:
        return "Trying to allocate space to an allocated segment (Alloc_Mem)";
    case 10598:
        return "Attempting to allocate a nonfree memory region (Take_Free)";
    case 10599:
        return "Fatal Disk error trying to read system code into memory";
    case 10600:
        return "Error attempting to make timer pipe";
    case 10601:
        return "Error from Kill_Object of an existing timer pipe";
    case 10602:
        return "Error from second Make Pipe to make timer pipe";
    case 10603:
        return "Error from Open to open timer pipe";
    case 10604:
        return "No syslocal space for head of timer list";
    case 10605:
        return "Error during allocate space for timer pipe, or interrupt from nonconfigured device";
    case 10609:
        return "Interrupt from nonconfigured device";
    case 10610:
        return "Error from info about timer pipe";
    case 10611:
        return "Spurious interrupt from floppy drive #2";
    case 10612:
        return "Spurious interrupt from floppy drive #1, or no syslocal space for timer list element";
    case 10613:
        return "Error from Read_Data of timer pipe";
    case 10614:
        return "Actual returned from Read_Data is not the same as requested from timer pipe";
    case 10615:
        return "Error from open of the receiver's event channel";
    case 10616:
        return "Error from Write_Event to the receiver's event channel";
    case 10617:
        return "Error from Close_Event_Chn on the receiver's pipe";
    case 10619:
        return "No sysglobal space for timer request block";
    case 10624:
        return "Attempt to shut down floppy disk controller while drive is still busy";
    case 10637:
        return "Not enough memory to initialize system timeout drives";
    case 10675:
        return "Spurious timeout on console driver";
    case 10699:
        return "Spurious timeout on parallel printer driver";
    case 10700:
        return "Mismatch between loader version number and Operating System version number";
    case 10701:
        return "OS exhausted its internal space during startup";
    case 10702:
        return "Cannot make system process";
    case 10703:
        return "Cannot kill pseudo-outer process";
    case 10704:
        return "Cannot create driver";
    case 10706:
        return "Cannot initialize floppy disk driver";
    case 10707:
        return "Cannot initialize the File System volume";
    case 10708:
        return "Hard disk mount table unreadable";
    case 10709:
        return "Cannot map screen data";
    case 10710:
        return "Too many slot-based devices";
    case 10724:
        return "The boot tracks do not know the right File System version";
    case 10725:
        return "Either damaged File System or damaged contents";
    case 10726:
        return "Boot device read failed";
    case 10727:
        return "The OS will not fit into the available memory";
    case 10728:
        return "SYSTEM.OS is missing";
    case 10729:
        return "SYSTEM.CONFIG is corrupt";
    case 10730:
        return "SYSTEM.OS is corrupt";
    case 10731:
        return "SYSTEM.DEBUG or SYSTEM.DEBUG2 is corrupt";
    case 10732:
        return "SYSTEM.LLD is corrupt";
    case 10733:
        return "Loader range error";
    case 10734:
        return "Wrong driver is found. For instance, storing a diskette loader on a ProFile";
    case 10735:
        return "SYSTEM.LLD is missing";
    case 10736:
        return "SYSTEM.UNPACK is missing";
    case 10737:
        return "Unpack of SYSTEM.OS with SYSTEM.UNPACK failed";
    case 10738:
        return "Checksum failure or too many hardware config changes (not sure of the text for this error.)";
    case 11176:
        return "Found a pending write request for a pipe while in Close_Object when it is called by the last writer of the pipe";
    case 11177:
        return "Found a pending read request for a pipe while in Close Object when it is called by the (only possible) reader of the pipe";
    case 11178:
        return "Found a pending read request for a pipe while in Read_Data from the pipe";
    case 11180:
        return "Found a pending write request for a pipe while in Write_Data to the pipe";
    case 11807:
        return "No disk in floppy drive";
    case 11820:
        return "Write-protect error on floppy drive";
    case 11822:
        return "Unable to clamp floppy drive";
    case 11824:
        return "Floppy drive write error";
    case 11882:
        return "Bad response from ProFile";
    case 11885:
        return "ProFile timeout error";
    case 11901:
        return "Call to Getspace or Relspace with a bad parameter, or free pool is bad";
    default:
        return "(unknown error)";
    }
