/* 
 * dhcpd.conf text user interface editor
 * Copyright (C) 2018  Victor C. Salas P. (aka nmag) <nmagko@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <uregex.h>
#include <aarray.h>
#include <dialog.h>
#include <errno.h>
#include <stdarg.h>
#include <dirent.h>

#define VERSION     0
#define SUBVERSION  1
#define PATCHLEVEL  2
#define AUTHOR      "Victor C. Salas P."
#define CRYEAR      2018
#define DEFPATH     "/etc/dhcp/"
#define DEFNAME     "dhcpd.conf"
#define DEFCONFIG   DEFPATH DEFNAME

/* Obtain configuration from file and put it into Asociative Array
 * Structure (AArray).
 */
struct AArray *get_dhcpd_config (const char *filename) {
  int fdes, ele_strarray, rid;
  FILE *fstream;
  char *gs = (char *) xmalloc (BUFSIZ);
  char *prefix_subnet, *prefix_host;
  char *prefix_plus_key;
  struct AArray *config = new_aa();
  char **strarray;
  if ( ( fdes = open(filename, O_RDONLY) ) == -1 ) {
    printf ("open %s, failed.\n", filename);
    free(gs);
    endwin();
    exit(EXIT_FAILURE);
  }
  asprintf(&prefix_subnet, "%s", "");
  asprintf(&prefix_host, "%s", "");
  asprintf(&prefix_plus_key, "%s", "");
  if ( ( fstream = fdopen(fdes, "r") ) != NULL ) {
#ifdef _DEBUG
    endwin();
#endif
    rid = 0;
    while ( fgets(gs, BUFSIZ - 1, fstream) != NULL ) {
      gs[BUFSIZ - 1] = 0x00; /* NULL */
      /* pre processing special chars here */
      gs = as_rex (gs, "#.*$", "", "");
      gs = as_rex (gs, "[;{}]", "", "g");
      gs = as_rex (gs, "[ \t\r\n]+", " ", "g");
      gs = as_rex (gs, "(^ | $)", "", "g");
      /* pre processing keywords here */
      gs = as_rex (gs, "^hardware ethernet", "hardware+ethernet", "");
      gs = as_rex (gs, "^option ", "option+", "");
      gs = as_rex (gs, "^subnet ", "subnet+", "");
      gs = as_rex (gs, "netmask ", "netmask+", "");
      gs = as_rex (gs, "^authoritative$", "authoritative ", "");
      /* last step: making key=value */
      gs = as_rex (gs, " ", "=", "");
      /* upload to memory just reserved keywords */
      if ( m_rex (gs,
		  "^(ddns-update-style|default-lease-time|max-lease-time|"
		  "shared-network|authoritative|log-facility|subnet|host|"
		  "hardware|fixed-address|option|range)", "")
	   ) {
	/* upload to memory just lines with equals mark */
	if ( m_rex (gs, "=", "") ) {
	  strarray = split ("=", "", gs, &ele_strarray);
	  /* hasn't it value? make a null value */
	  if ( ele_strarray < 2 ) {
	    strarray[1] = xmalloc(1);
	    strarray[1][0] = 0x00; /* NULL */
	    ele_strarray++;
	  }
	  free(prefix_plus_key);
	  if ( m_rex (strarray[0], "^(subnet|option|range)", "") ) {
	    if ( m_rex (strarray[0], "^subnet", "") ) {
	      rid = 0;
	      free(prefix_subnet);
	      asprintf(&prefix_subnet, "%s", "");
	    }
	    free(prefix_host);
	    asprintf(&prefix_host, "%s", "");
	  }
	  if ( m_rex (strarray[0], "^range", "") ) {
	    asprintf(&prefix_plus_key, "%s%s%s%i",
		     prefix_subnet, prefix_host, strarray[0], rid);
	    rid++;
	  } else {
	    if ( m_rex (strarray[0], "^option", "") && ( strlen(prefix_subnet) == 0 ) ) {
	      asprintf(&prefix_plus_key, "%s", "");
	      free_double_pointer(strarray, ele_strarray);
	      goto skipoption;
	    } else {
	      asprintf(&prefix_plus_key, "%s%s%s",
		       prefix_subnet, prefix_host, strarray[0]);
	    }
	  }
	  if ( ! m_rex (strarray[0], "^host", "") ) {
	    put_aa(config, prefix_plus_key, strarray[1]);
#if defined( _DEBUG ) && !defined( _INFO )
	    printf("%s=%s\n", prefix_plus_key, strarray[1]);
#endif
	  }
	  if ( m_rex (strarray[0], "^subnet", "") ) {
	    free(prefix_subnet);
	    asprintf(&prefix_subnet, "%s/", strarray[0]);
	    free(prefix_host);
	    asprintf(&prefix_host, "%s", "");
	  }
	  if ( m_rex (strarray[0], "^host", "") ) {
	    free(prefix_host);
	    asprintf(&prefix_host, "%s/", strarray[1]);
	  }
	  free_double_pointer(strarray, ele_strarray);
	}
      }
    skipoption:
      /* reallocating gs memory because as_rex reallocates it */
      gs = (char *) xrealloc (gs, BUFSIZ);
    }
#ifdef _DEBUG
# ifndef _INFO
    printf("\nPress any key..."); getchar();
# endif
    (void) initscr();
#endif
    fclose(fstream);
  } else {
    printf("reading %s, failed.\n", filename);
    free(gs);
    close(fdes);
    endwin();
    exit(EXIT_FAILURE);
  }
  free(prefix_subnet);
  free(prefix_host);
  free(prefix_plus_key);
  free(gs);
  close(fdes);
  return(config);
}

char **manual_fast_menu (int *menusz, ...) {
  va_list strings;
  char *vas, **menu;
  va_start (strings, menusz);
  *menusz = 0;
  menu = xmalloc(sizeof(menu));
  while ( ( vas = va_arg(strings, char *) ) != NULL ) {
    menu[(*menusz)++] = savestring(vas);
    menu = xrealloc(menu, sizeof(menu) * ((*menusz) + 1));
  }
  va_end (strings);
  return menu;
}

int filecopy (const char *fnin, const char *fnout) {
  int inputFd, outputFd, openFlags;
  mode_t filePerms;
  ssize_t numRead;
  char buf[BUFSIZ];
 
  /* Open input and output files */
  inputFd = open(fnin, O_RDONLY);
  if (inputFd == -1)
    return inputFd;
  openFlags = O_CREAT | O_WRONLY | O_TRUNC;
  // filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; /* rw-rw-rw- */
  filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; /* rw-r--r-- */
  outputFd = open(fnout, openFlags, filePerms);
  if (outputFd == -1)
    return outputFd;
 
  /* Transfer data until we encounter end of input or an error */
  while ((numRead = read(inputFd, buf, BUFSIZ)) > 0)
    if (write(outputFd, buf, numRead) != numRead)
      return EXIT_FAILURE;
  if (numRead == -1)
    return numRead;
  if (close(inputFd) == -1)
    return EXIT_FAILURE;
  if (close(outputFd) == -1)
    return EXIT_FAILURE;
 
  return EXIT_SUCCESS;
}

char **lsbkdir_fast_menu (int *menusz) {
  DIR *dp;
  struct dirent *ep;
  char **menu;
  *menusz = 0;
  char *bs;
  dp = opendir (DEFPATH);
  if (dp != NULL) {
    menu = xmalloc(sizeof(menu));
    while ( (ep = readdir (dp)) != NULL ) {
      if ( m_rex(ep->d_name, DEFNAME "-[0-9]+-[0-9]", "") ) {
	menu[(*menusz)++] = savestring(ep->d_name);
	menu = xrealloc(menu, sizeof(menu) * ((*menusz) + 1));
	bs = s_rex(ep->d_name, DEFNAME "-", "", "");
	asprintf(&(menu[(*menusz)++]), "%c%c%c%c/%c%c/%c%c %c%c:%c%c:%c%c",
		 bs[0], bs[1], bs[2], bs[3], bs[4], bs[5], bs[6],
		 bs[7], bs[9], bs[10], bs[11], bs[12], bs[13], bs[14]);
	menu = xrealloc(menu, sizeof(menu) * ((*menusz) + 1));
      }
    }
    (void) closedir (dp);
  } else {
    return NULL;
  }
  return menu;
}

int save_dhcpd_config (const char *filename, struct AArray *config, long int k_lim, char **key) {
  char **subnet;
  int fdes, i_key, i_subnet, subnetsz = 0, is_shared_network = 0;
  FILE *fstream;
  char *fstrm, *fstrm_temp, *sk, *sv, *fstrm_opts, *suffix, *filename_suffix, *tabs;
  struct tm *s_suffix;
  time_t tm_t;
  tm_t = time(NULL);
  s_suffix = localtime(&tm_t);
  suffix = xmalloc(18);
  if ( strftime(suffix, 17, "-%Y%m%d-%H%M%S", s_suffix) == 0 ) {
    free(suffix);
    return EXIT_FAILURE;
  } else {
    asprintf(&filename_suffix, "%s%s", filename, suffix);
    free(suffix);
    if ( filecopy (filename, filename_suffix) != 0 ) {
      free(filename_suffix);
      return EXIT_FAILURE;
    }
    free(filename_suffix);
  }
  asprintf(&fstrm, "# %s: dhcpd.conf auto generated\n", program_invocation_short_name);
  subnet = xmalloc(sizeof(subnet));
  for ( i_key = 0; i_key < k_lim; i_key++ ) {
    if ( ! m_rex(key[i_key], "/", "" ) ) {
      if ( m_rex(key[i_key], "^subnet", "" ) ) {
	subnet[subnetsz++] = savestring(key[i_key]);
	subnet = xrealloc(subnet, sizeof(subnet) * (subnetsz + 1));
      } else {
	fstrm_temp = fstrm;
	if ( m_rex(key[i_key], "^shared-network$", "") ) {
	  /* especific rule just for shared-network reserved word */
	  is_shared_network = 1;
	  asprintf(&fstrm, "%s# %s: You have to use dot1q instead shared network.\n%s %s {\n", fstrm_temp, program_invocation_short_name, key[i_key], get_aa(config, key[i_key]));
	} else
	if ( m_rex(key[i_key], "^authoritative$", "") ) {
	  /* especific rule just for authoritative reserved word */
	  asprintf(&fstrm, "%s%s;\n", fstrm_temp, key[i_key]);
	} else {
	  asprintf(&fstrm, "%s%s %s;\n", fstrm_temp, key[i_key], get_aa(config, key[i_key]));
	}
	free(fstrm_temp);
      }
    }
  }
  if (is_shared_network) {
    asprintf(&tabs, "  ");
  } else {
    asprintf(&tabs, "");
  }
  for ( i_subnet = 0; i_subnet < subnetsz; i_subnet++ ) {
    sk = savestring(subnet[i_subnet]);
    sk = as_rex(sk, "\\+", " ", "g" );
    sv = savestring(get_aa(config, subnet[i_subnet]));
    sv = as_rex(sv, "\\+", " ", "g" );
    fstrm_temp = fstrm;
    asprintf(&fstrm, "%s%s%s %s {\n", fstrm_temp, tabs, sk, sv);
    free(fstrm_temp);
    free(sk);
    free(sv);
    subnet[i_subnet] = as_rex(subnet[i_subnet], "\\+", "\\+", "g");
    subnet[i_subnet] = as_rex(subnet[i_subnet], "\\.", "\\.", "g");
    subnet[i_subnet] = as_rex(subnet[i_subnet], "^", "^", "");
    subnet[i_subnet] = as_rex(subnet[i_subnet], "$", "/", "");
    asprintf(&fstrm_opts, "  %s", tabs);
    for ( i_key = 0; i_key < k_lim; i_key++ ) {
      if ( m_rex(key[i_key], subnet[i_subnet], "" ) &&
	   m_rex(key[i_key], "/hardware\\+ethernet", "" ) ) {
	sk = s_rex(key[i_key], "/hardware\\+ethernet", "/fixed-address", "");
	sv = s_rex(key[i_key], "/[^/]+$", "", "");
	sv = as_rex(sv, "^[^/]+/", "", "");
	fstrm_temp = fstrm;
	asprintf(&fstrm,
		 "%s%s  host %s {\n%s    hardware ethernet %s;\n%s    fixed-address %s;\n%s  }\n",
		 fstrm_temp, tabs, sv, tabs, get_aa(config, key[i_key]), tabs, get_aa(config, sk), tabs);
	free(fstrm_temp);
	free(sk);
	free(sv);
      } else if ( m_rex(key[i_key], subnet[i_subnet], "" ) &&
		  ( m_rex(key[i_key], "/range", "") ||
		    m_rex(key[i_key], "/option", "") ) ) {
	sk = s_rex(key[i_key], "^[^/]+/", "", "");
	sk = as_rex(sk, "[0-9]+$", "", "");
	sk = as_rex(sk, "\\+", " ", "g");
	fstrm_temp = fstrm_opts;
	asprintf(&fstrm_opts, "%s%s %s;\n  %s", fstrm_temp, sk, get_aa(config, key[i_key]), tabs);
	free(fstrm_temp);
	free(sk);
      }
    }
    fstrm_opts = as_rex(fstrm_opts, "  $", "", "");
    fstrm_temp = fstrm;
    asprintf(&fstrm, "%s%s}\n", fstrm_temp, fstrm_opts);
    free(fstrm_temp);
    free(fstrm_opts);
  }
  if ( is_shared_network ) {
    fstrm_temp = fstrm;
    asprintf(&fstrm, "%s}\n", fstrm_temp);
    free(fstrm_temp);
  }
#ifdef _DEBUG
  endwin();
# ifndef _INFO
  printf("%s", fstrm);
# endif
#endif
  free_double_pointer(subnet, subnetsz);
  if ( ( fdes = open(filename, O_WRONLY | O_TRUNC) ) == -1 ) {
#if defined( _DEBUG ) && !defined( _INFO )
    printf ("open %s, failed.\n", filename);
#endif
    return EXIT_FAILURE;
  }
  if ( ( fstream = fdopen(fdes, "w") ) != NULL ) {
    fprintf(fstream, "%s", fstrm);
    fclose(fstream);
  }
  free(fstrm);
  close(fdes);
#ifdef _DEBUG
  (void) initscr();
#endif
  return EXIT_SUCCESS;
}

int main (int argc, char *argv[]) {
  char **key, **menu, **fminput;
  long int k_lim;
  int idx, fmcount;
  int menusz, rok;
  struct AArray *config;
  char *title, *mesg;
  char *choosenkey, *choosenkey_regcomp, *choosenvalue,
    *choosenkey_temp, *choosenkey_hw, *choosenkey_ip;

  asprintf(&title, " %s %d.%d.%d (C) %i  %s ",
	   program_invocation_short_name,
	   VERSION, SUBVERSION, PATCHLEVEL, CRYEAR, AUTHOR);

  (void) initscr();
  init_dialog(stdin, stderr);

  dialog_msgbox(title,
		"\ndhcpd.conf text user interface editor\n"
		"Copyright (C) 2018  Victor C. Salas P. (aka nmag) <nmagko@gmail.com>\n\n"
		"This program is free software: you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation, either version 3 of the License, or\n"
		"(at your option) any later version.\n\n"
		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		"GNU General Public License for more details.\n\n"
		"You should have received a copy of the GNU General Public License\n"
		"along with this program.  If not, see <http://www.gnu.org/licenses/>.\n",
		22, 72, true);
  config = get_dhcpd_config(DEFCONFIG);
 startagain:
  key = keys_aa(config, &k_lim);
  menu = manual_fast_menu(&menusz,
			  "Subnetworks", "Handle subnetworks",
			  "Options", "Handle global options",
			  "Restore", "Restore previous configurations",
			  "Save", "Save changes and exit",
			  NULL);
  asprintf(&mesg, "What to do?");
  if (dialog_vars.input_result) dialog_vars.input_result[0] = '\0';
  rok = dialog_menu(title,
		    mesg,
		    22, 72, 17,
		    menusz / 2, menu);
  free(mesg);
  if ( rok == 0 ) {
    if ( m_rex(dialog_vars.input_result, "Subnetworks", "") ) {
      /* Subnetworks */
      free_double_pointer(menu, menusz);
      menusz = 0;
      menu = xmalloc(sizeof(menu));
      for ( idx = 0; idx < k_lim; idx++ ) {
	if ( ! m_rex(key[idx], "/", "" ) ) {
	  if ( m_rex(key[idx], "^subnet", "" ) ) {
	    menu[menusz++] = savestring(key[idx]);
	    menu = xrealloc(menu, sizeof(menu) * (menusz + 1));
	    menu[menusz++] = join("/", key[idx], get_aa(config, key[idx]), NULL);
	    menu[menusz-1] = as_rex(menu[menusz-1], "(subnet|netmask)\\+", "", "g");
	    menu = xrealloc(menu, sizeof(menu) * (menusz + 1));
	  }
	}
      }
      menu[menusz++] = savestring("Create subnet");
      menu = xrealloc(menu, sizeof(menu) * (menusz + 1));
      menu[menusz++] = savestring("Create a new subnetwork");
      menu = xrealloc(menu, sizeof(menu) * (menusz + 1));
      if (dialog_vars.input_result) dialog_vars.input_result[0] = '\0';
      rok = dialog_menu(title,
			"Choose subnetwork:",
			22, 72, 17,
			menusz / 2, menu);
      if ( rok == 0 ) {
	asprintf(&choosenkey, "%s", dialog_vars.input_result);
	if ( m_rex(choosenkey, "^Create subnet", "") ) {
	  /* Create new subnetwork */
	  free_double_pointer(menu, menusz);
	  menu = manual_fast_menu(&menusz,
				  "Network     :", "1", "1", "", "1", "15", "15", "0",
				  "Subnet-mask :", "2", "1", "", "2", "15", "15", "0",
				  "Broadcast   :", "3", "1", "", "3", "15", "15", "0",
				  "Gateway     :", "4", "1", "", "4", "15", "15", "0",
				  "Nameserver  :", "5", "1", "", "5", "15", "15", "0",
				  NULL);
	  asprintf(&mesg, "Subnetwork values:");
	  if (dialog_vars.input_result) dialog_vars.input_result[0] = '\0';
	  rok = dialog_form(title,
			    mesg,
			    22, 72, 14,
			    menusz / 8, menu);
	  free(mesg);
	  if ( rok == 0 ) {
	    fminput = split("\n", "", dialog_vars.input_result, &fmcount);
	    fminput[0] = as_rex(fminput[0], "^ +| +$", "", "g");
	    fminput[0] = as_rex(fminput[0], "[^0-9]+", ".", "g");
	    fminput[1] = as_rex(fminput[1], "^ +| +$", "", "g");
	    fminput[1] = as_rex(fminput[1], "[^0-9]+", ".", "g");
	    fminput[2] = as_rex(fminput[2], "^ +| +$", "", "g");
	    fminput[2] = as_rex(fminput[2], "[^0-9]+", ".", "g");
	    fminput[3] = as_rex(fminput[3], "^ +| +$", "", "g");
	    fminput[3] = as_rex(fminput[3], "[^0-9]+", ".", "g");
	    fminput[4] = as_rex(fminput[4], "^ +| +$", "", "g");
	    fminput[4] = as_rex(fminput[4], "[^0-9]+", ".", "g");
	    asprintf(&choosenkey_temp, "subnet+%s", fminput[0]);
	    asprintf(&choosenvalue, "netmask+%s", fminput[1]);
	    put_aa(config, choosenkey_temp, choosenvalue);
	    free(choosenkey_temp);
	    free(choosenvalue);
	    asprintf(&choosenkey_temp, "subnet+%s/option+routers", fminput[0]);
	    asprintf(&choosenvalue, "%s", fminput[3]);
	    put_aa(config, choosenkey_temp, choosenvalue);
	    free(choosenkey_temp);
	    free(choosenvalue);
	    asprintf(&choosenkey_temp, "subnet+%s/option+subnet-mask", fminput[0]);
	    asprintf(&choosenvalue, "%s", fminput[1]);
	    put_aa(config, choosenkey_temp, choosenvalue);
	    free(choosenkey_temp);
	    free(choosenvalue);
	    asprintf(&choosenkey_temp, "subnet+%s/option+broadcast-address", fminput[0]);
	    asprintf(&choosenvalue, "%s", fminput[2]);
	    put_aa(config, choosenkey_temp, choosenvalue);
	    free(choosenkey_temp);
	    free(choosenvalue);
	    asprintf(&choosenkey_temp, "subnet+%s/option+domain-name-servers", fminput[0]);
	    asprintf(&choosenvalue, "%s", fminput[4]);
	    put_aa(config, choosenkey_temp, choosenvalue);
	    free(choosenkey_temp);
	    free(choosenvalue);
	    free_double_pointer(fminput, fmcount);
	  }
	  free(choosenkey);
	  free_double_pointer(menu, menusz);
	  free(key);
	  goto startagain;
	}
#ifdef _DEBUG
	endwin();
	printf("%s", dialog_vars.input_result);
# ifndef _INFO
	printf("\n\nPress any key..."); getchar();
# endif
	(void) initscr();
#endif
	free_double_pointer(menu, menusz);
	menu = manual_fast_menu(&menusz,
				"option", "Subnetwork options",
				"range", "Automatic subnetwork DHCP Range",
				"host", "Subnetwork hosts",
				NULL);
	asprintf(&mesg, "What to handle in %s?", dialog_vars.input_result);
	if (dialog_vars.input_result) dialog_vars.input_result[0] = '\0';
	rok = dialog_menu(title,
			  mesg,
			  22, 72, 17,
			  menusz / 2, menu);
	free(mesg);
	if ( rok == 0 ) {
	  asprintf(&choosenkey, "%s/%s", choosenkey, dialog_vars.input_result);
	  choosenkey = as_rex(choosenkey, "/host$", "/", "");
#ifdef _DEBUG
	  endwin();
	  printf("%s", dialog_vars.input_result);
# ifndef _INFO
	  printf("\n\nPress any key..."); getchar();
# endif
	  (void) initscr();
#endif
	  if ( m_rex(choosenkey, "/$", "") ) {
	    /* Hosts */
	    free_double_pointer(menu, menusz);
	    menu = manual_fast_menu(&menusz,
				    "Create", "Create a new host",
				    "Remove", "Remove host",
				    "Edit", "Modify host's values",
				    NULL);
	    asprintf(&mesg, "What do you wanna do with hosts?");
	    if (dialog_vars.input_result) dialog_vars.input_result[0] = '\0';
	    rok = dialog_menu(title,
			      mesg,
			      22, 72, 17,
			      menusz / 2, menu);
	    free(mesg);
	    if ( rok == 0 ) {
#ifdef _DEBUG
	      endwin();
	      printf("%s", dialog_vars.input_result);
# ifndef _INFO
	      printf("\n\nPress any key..."); getchar();
# endif
	      (void) initscr();
#endif
	      switch (dialog_vars.input_result[0]) {
	      case 'C' :
		/* Create new entry */
		free_double_pointer(menu, menusz);
		menu = manual_fast_menu(&menusz,
					"Hostname    :", "1", "1", "", "1", "15", "32", "0",
					"MAC Address :", "2", "1", "", "2", "15", "17", "0",
					"IP Address  :", "3", "1", "", "3", "15", "15", "0",
					NULL);
		asprintf(&mesg, "Just alfanumeric and dash allowed by Hostname, MAC address colon separated:");
		if (dialog_vars.input_result) dialog_vars.input_result[0] = '\0';
		rok = dialog_form(title,
				  mesg,
				  22, 72, 14,
				  menusz / 8, menu);
		free(mesg);
		if ( rok == 0 ) {
		  fminput = split("\n", "", dialog_vars.input_result, &fmcount);
		  fminput[0] = as_rex(fminput[0], "[^A-Za-z0-9-]", "", "g");
		  fminput[1] = as_rex(fminput[1], "[\\.-]+", ":", "g");
		  fminput[1] = as_rex(fminput[1], "[^A-Fa-f0-9:]", "", "g");
		  fminput[2] = as_rex(fminput[2], "^ +| +$", "", "g");
		  fminput[2] = as_rex(fminput[2], "[^0-9]+", ".", "g");
#ifdef _DEBUG
		  endwin();
		  (fmcount == 3 ) ? printf("%s, %s, %s", fminput[0], fminput[1], fminput[2]) : printf("%i", fmcount);
# ifndef _INFO
		  printf("\n\nPress any key..."); getchar();
# endif
		  (void) initscr();
#endif
		  choosenkey_temp = join("", choosenkey, fminput[0], "/hardware+ethernet", NULL);
		  put_aa(config, choosenkey_temp, fminput[1]);
		  free(choosenkey_temp);
		  choosenkey_temp = join("", choosenkey, fminput[0], "/fixed-address", NULL);
		  put_aa(config, choosenkey_temp, fminput[2]);
		  free(choosenkey_temp);
		  free_double_pointer(fminput, fmcount);
#ifdef _DEBUG
		} else {
		  endwin();
		  printf("Canceled.");
# ifndef _INFO
		  printf("\n\nPress any key..."); getchar();
# endif
		  (void) initscr();
#endif
		}
		free(choosenkey);
		free_double_pointer(menu, menusz);
		free(key);
		goto startagain;
		break;
	      case 'R' :
		/* Remove entry */
		free_double_pointer(menu, menusz);
		menusz = 0;
		menu = xmalloc(sizeof(menu));
		choosenkey_regcomp = s_rex(choosenkey, "\\+", "\\+", "g");
		choosenkey_regcomp = as_rex(choosenkey_regcomp, "\\.", "\\.", "g");
#ifdef _DEBUG
		endwin();
		printf("[%li]{%s}", k_lim, choosenkey_regcomp);
#endif
		for ( idx = 0; idx < k_lim; idx++ ) {
		  if ( m_rex(key[idx], choosenkey_regcomp, "" ) &&
		       m_rex(key[idx], "/hardware\\+ethernet", "" ) ) {
		    /* a little random problem here with allocation? */
		    menu[menusz++] = savestring(key[idx]);
		    menu[menusz-1] = as_rex(menu[menusz-1], choosenkey_regcomp, "", "");
		    menu[menusz-1] = as_rex(menu[menusz-1], "/hardware\\+ethernet", "", "");
		    menu = xrealloc(menu, sizeof(menu) * (menusz + 1));
		    choosenkey_temp = savestring(key[idx]);
		    choosenkey_temp = as_rex(choosenkey_temp, "/hardware\\+ethernet", "/fixed-address", "");
		    menu[menusz++] = join(" ", get_aa(config, key[idx]), get_aa(config, choosenkey_temp), NULL);
		    free(choosenkey_temp);
		    menu = xrealloc(menu, sizeof(menu) * (menusz + 1));
#if defined( _DEBUG ) && !defined( _INFO )
		    printf("\n%s=%s", menu[menusz-2], menu[menusz-1]);
#endif
		  }
		}
		free(choosenkey_regcomp);
#ifdef _DEBUG
# ifndef _INFO
		printf("\n\nPress any key..."); getchar();
# endif
		(void) initscr();
#endif
		asprintf(&mesg, "Choose one host of %sto remove:", choosenkey);
		mesg = as_rex(mesg, "[+/]", " ", "g");
		if (dialog_vars.input_result) dialog_vars.input_result[0] = '\0';
		rok = dialog_menu(title,
				  mesg,
				  22, 72, 17,
				  menusz / 2, menu);
		free(mesg);
		if ( rok == 0 ) {
		  choosenkey_temp = join("", choosenkey, dialog_vars.input_result, "/hardware+ethernet", NULL);
		  delete_aa(config, choosenkey_temp);
		  free(choosenkey_temp);
		  choosenkey_temp = join("", choosenkey, dialog_vars.input_result, "/fixed-address", NULL);
		  delete_aa(config, choosenkey_temp);
		  free(choosenkey_temp);
#ifdef _DEBUG
		} else {
		  endwin();
		  printf("Canceled.");
# ifndef _INFO
		  printf("\n\nPress any key..."); getchar();
# endif
		  (void) initscr();
#endif
		}
		free(choosenkey);
		free_double_pointer(menu, menusz);
		free(key);
		goto startagain;
		break;
	      case 'E' :
		/* Entry edit */
		free_double_pointer(menu, menusz);
		menusz = 0;
		menu = xmalloc(sizeof(menu));
		choosenkey_regcomp = s_rex(choosenkey, "\\+", "\\+", "g");
		choosenkey_regcomp = as_rex(choosenkey_regcomp, "\\.", "\\.", "g");
#ifdef _DEBUG
		endwin();
		printf("[%li]{%s}", k_lim, choosenkey_regcomp);
#endif
		for ( idx = 0; idx < k_lim; idx++ ) {
		  if ( m_rex(key[idx], choosenkey_regcomp, "" ) &&
		       m_rex(key[idx], "/hardware\\+ethernet", "" ) ) {
		    /* a little random problem here with allocation? */
		    menu[menusz++] = savestring(key[idx]);
		    menu[menusz-1] = as_rex(menu[menusz-1], choosenkey_regcomp, "", "");
		    menu[menusz-1] = as_rex(menu[menusz-1], "/hardware\\+ethernet", "", "");
		    menu = xrealloc(menu, sizeof(menu) * (menusz + 1));
		    choosenkey_temp = savestring(key[idx]);
		    choosenkey_temp = as_rex(choosenkey_temp, "/hardware\\+ethernet", "/fixed-address", "");
		    menu[menusz++] = join(" ", get_aa(config, key[idx]), get_aa(config, choosenkey_temp), NULL);
		    free(choosenkey_temp);
		    menu = xrealloc(menu, sizeof(menu) * (menusz + 1));
#if defined( _DEBUG ) && !defined( _INFO )
		    printf("\n%s=%s", menu[menusz-2], menu[menusz-1]);
#endif
		  }
		}
		free(choosenkey_regcomp);
#ifdef _DEBUG
# ifndef _INFO
		printf("\n\nPress any key..."); getchar();
# endif
		(void) initscr();
#endif
		asprintf(&mesg, "Choose one host of %sto change entry:", choosenkey);
		mesg = as_rex(mesg, "[+/]", " ", "g");
		if (dialog_vars.input_result) dialog_vars.input_result[0] = '\0';
		rok = dialog_menu(title,
				  mesg,
				  22, 72, 17,
				  menusz / 2, menu);
		free(mesg);
		if ( rok == 0 ) {
		  free_double_pointer(menu, menusz);
		  choosenkey_hw = join("", choosenkey, dialog_vars.input_result, "/hardware+ethernet", NULL);
		  choosenkey_ip = join("", choosenkey, dialog_vars.input_result, "/fixed-address", NULL);
		  menu = manual_fast_menu(&menusz,
					  "MAC Address :", "1", "1", get_aa(config, choosenkey_hw), "1", "15", "17", "0",
					  "IP Address  :", "2", "1", get_aa(config, choosenkey_ip), "2", "15", "15", "0",
					  NULL);
		  asprintf(&mesg, "%s selected:", dialog_vars.input_result);
		  if (dialog_vars.input_result) dialog_vars.input_result[0] = '\0';
		  rok = dialog_form(title,
				    mesg,
				    22, 72, 14,
				    menusz / 8, menu);
		  free(mesg);
		  if ( rok == 0 ) {
		    fminput = split("\n", "", dialog_vars.input_result, &fmcount);
		    fminput[0] = as_rex(fminput[0], "[\\.-]+", ":", "g");
		    fminput[0] = as_rex(fminput[0], "[^A-Fa-f0-9:]", "", "g");
		    fminput[1] = as_rex(fminput[1], "^ +| +$", "", "g");
		    fminput[1] = as_rex(fminput[1], "[^0-9]+", ".", "g");
		    put_aa(config, choosenkey_hw, fminput[0]);
		    put_aa(config, choosenkey_ip, fminput[1]);
		    free_double_pointer(fminput, fmcount);
#ifdef _DEBUG
		  } else {
		    endwin();
		    printf("Canceled.");
# ifndef _INFO
		    printf("\n\nPress any key..."); getchar();
# endif
		    (void) initscr();
#endif
		  }
		  free(choosenkey);
		  free(choosenkey_hw);
		  free(choosenkey_ip);
		  free_double_pointer(menu, menusz);
		  free(key);
		  goto startagain;
#ifdef _DEBUG
		} else {
		  endwin();
		  printf("Canceled.");
# ifndef _INFO
		  printf("\n\nPress any key..."); getchar();
# endif
		  (void) initscr();
#endif
		}
		free(choosenkey);
		free_double_pointer(menu, menusz);
		free(key);
		goto startagain;
		break;
	      }
	    } else {
	      free(choosenkey);
#ifdef _DEBUG
	      endwin();
	      printf("Canceled.");
# ifndef _INFO
	      printf("\n\nPress any key..."); getchar();
# endif
	      (void) initscr();
#endif
	      free_double_pointer(menu, menusz);
	      free(key);
	      goto startagain;
	    }
	  } else if ( m_rex(choosenkey, "/option$", "" ) ) {
	    /* Subnet options */
	    free_double_pointer(menu, menusz);
	    menusz = 0;
	    menu = xmalloc(sizeof(menu));
	    choosenkey_regcomp = s_rex(choosenkey, "\\+", "\\+", "g");
	    choosenkey_regcomp = as_rex(choosenkey_regcomp, "\\.", "\\.", "g");
#ifdef _DEBUG
	    endwin();
	    printf("[%li]{%s}", k_lim, choosenkey_regcomp);
#endif
	    for ( idx = 0; idx < k_lim; idx++ ) {
	      if ( m_rex(key[idx], choosenkey_regcomp, "" ) ) {
		/* a little random problem here with allocation? */
		menu[menusz++] = savestring(key[idx]);
		menu[menusz-1] = as_rex(menu[menusz-1], ".*\\+", "", "g");
		menu = xrealloc(menu, sizeof(menu) * (menusz + 1));
		menu[menusz++] = savestring(get_aa(config, key[idx]));
		menu = xrealloc(menu, sizeof(menu) * (menusz + 1));
#if defined( _DEBUG ) && !defined( _INFO )
		printf("\n%s=%s", menu[menusz-2], menu[menusz-1]);
#endif
	      }
	    }
	    free(choosenkey_regcomp);
#ifdef _DEBUG
# ifndef _INFO
	    printf("\n\nPress any key..."); getchar();
# endif
	    (void) initscr();
#endif
	    asprintf(&mesg, "Choose %s:", choosenkey);
	    mesg = as_rex(mesg, "[+/]", " ", "g");
	    if (dialog_vars.input_result) dialog_vars.input_result[0] = '\0';
	    rok = dialog_menu(title,
			      mesg,
			      22, 72, 17,
			      menusz / 2, menu);
	    free(mesg);
	    if ( rok == 0 ) {
	      asprintf(&choosenkey_temp, "%s+%s", choosenkey, dialog_vars.input_result);
	      free(choosenkey);
	      choosenkey = savestring(choosenkey_temp);
	      free(choosenkey_temp);
	      choosenvalue = savestring(get_aa(config, choosenkey));
	      asprintf(&mesg, "%s selected:", choosenkey);
	      rok = dialog_inputbox(title,
				    mesg,
				    22, 72,
				    choosenvalue, 0);
	      free(mesg);
	      if ( rok == 0 ) {
		put_aa(config, choosenkey, dialog_vars.input_result);
#ifdef _DEBUG
	      } else {
		endwin();
		printf("Canceled.");
# ifndef _INFO
		printf("\n\nPress any key..."); getchar();
# endif
		(void) initscr();
#endif
	      }
	      free(choosenkey);
	      free(choosenvalue);
	      free_double_pointer(menu, menusz);
	      free(key);
	      goto startagain;
	    } else {
	      free(choosenkey);
#ifdef _DEBUG
	      endwin();
	      printf("Canceled.");
# ifndef _INFO
	      printf("\n\nPress any key..."); getchar();
# endif
	      (void) initscr();
#endif
	      free_double_pointer(menu, menusz);
	      free(key);
	      goto startagain;
	    }
	  } else if ( m_rex(choosenkey, "/range$", "") ) {
	    /* Automatic subnet range */
	    free_double_pointer(menu, menusz);
	    menusz = 0;
	    menu = xmalloc(sizeof(menu));
	    choosenkey_regcomp = s_rex(choosenkey, "\\+", "\\+", "g");
	    choosenkey_regcomp = as_rex(choosenkey_regcomp, "\\.", "\\.", "g");
#ifdef _DEBUG
	    endwin();
	    printf("[%li]{%s}", k_lim, choosenkey_regcomp);
#endif
	    for ( idx = 0; idx < k_lim; idx++ ) {
	      if ( m_rex(key[idx], choosenkey_regcomp, "" ) ) {
		menu[menusz++] = savestring(key[idx]);
		menu = xrealloc(menu, sizeof(menu) * (menusz + 1));
		menu[menusz++] = savestring(get_aa(config, key[idx]));
		menu = xrealloc(menu, sizeof(menu) * (menusz + 1));
#if defined( _DEBUG ) && !defined( _INFO )
		printf("\n%s=%s", menu[menusz-2], menu[menusz-1]);
#endif
	      }
	    }
	    free(choosenkey_regcomp);
#ifdef _DEBUG
# ifndef _INFO
	    printf("\n\nPress any key..."); getchar();
# endif
	    (void) initscr();
#endif
	    rok = 0;
	    //free(choosenkey);
	    if ( menusz / 2 > 1 ) {
	      free(choosenkey);
	      asprintf(&mesg, "IP range, choose one:");
	      if (dialog_vars.input_result) dialog_vars.input_result[0] = '\0';
	      rok = dialog_menu(title,
				mesg,
				22, 72, 17,
				menusz / 2, menu);
	      free(mesg);
	      if ( rok == 0 ) {
		choosenkey = savestring(dialog_vars.input_result);
		choosenvalue = savestring(get_aa(config, choosenkey));
	      } else {
#ifdef _DEBUG
		endwin();
		printf("Canceled.");
# ifndef _INFO
		printf("\n\nPress any key..."); getchar();
# endif
		(void) initscr();
#endif
		free_double_pointer(menu, menusz);
		free(key);
		goto startagain;
	      }
	    }
	    if ( menusz / 2 == 1 ) {
	      free(choosenkey);
	      choosenkey = savestring(menu[menusz-2]);
	      choosenvalue = savestring(menu[menusz-1]);
	    }
	    if ( menusz / 2 < 1 ) {
	      //free(choosenkey);
	      /* selection error, automatic cancel (rok = 1;) */
	      choosenvalue = s_rex(choosenkey, "subnet\\+", "", "");
	      choosenvalue = as_rex(choosenvalue, "\\.[^\\.]+$", ".", "");
	      asprintf(&mesg, "IP range, first IP and last IP blankspace separated:");
	      rok = dialog_inputbox(title,
				    mesg,
				    22, 72,
				    choosenvalue, 0);
	      free(mesg);
	      if ( rok == 0 ) {
		choosenkey = as_rex(choosenkey, "$", "0", "");
		put_aa(config, choosenkey, dialog_vars.input_result);
#ifdef _DEBUG
	      } else {
		endwin();
		printf("Canceled.");
# ifndef _INFO
		printf("\n\nPress any key..."); getchar();
# endif
		(void) initscr();
#endif
	      }
	      free(choosenkey);
	      free(choosenvalue);
	      free_double_pointer(menu, menusz);
	      free(key);
	      goto startagain;
	    }
	    if ( rok == 0 ) {
	      asprintf(&mesg, "IP range, first IP and last IP blankspace separated:");
	      rok = dialog_inputbox(title,
				    mesg,
				    22, 72,
				    choosenvalue, 0);
	      free(mesg);
	      if ( rok == 0 ) {
		put_aa(config, choosenkey, dialog_vars.input_result);
#ifdef _DEBUG
	      } else {
		endwin();
		printf("Canceled.");
# ifndef _INFO
		printf("\n\nPress any key..."); getchar();
# endif
		(void) initscr();
#endif
	      }
	      free(choosenkey);
	      free(choosenvalue);
	      free_double_pointer(menu, menusz);
	      free(key);
	      goto startagain;
	    }
	  }
	} else {
	  free(choosenkey);
#ifdef _DEBUG
	  endwin();
	  printf("Canceled.");
# ifndef _INFO
	  printf("\n\nPress any key..."); getchar();
# endif
	  (void) initscr();
#endif
	  free_double_pointer(menu, menusz);
	  free(key);
	  goto startagain;
	}

      } else {
#ifdef _DEBUG
	endwin();
	printf("Canceled.");
# ifndef _INFO
	printf("\n\nPress any key..."); getchar();
# endif
	(void) initscr();
#endif
	free_double_pointer(menu, menusz);
	free(key);
	goto startagain;
      }
    } else if ( m_rex(dialog_vars.input_result, "Options", "") ) {
      /* Global options */
      free_double_pointer(menu, menusz);
      menusz = 0;
      menu = xmalloc(sizeof(menu));
#ifdef _DEBUG
      endwin();
#endif
      for ( idx = 0; idx < k_lim; idx++ ) {
	if ( ! m_rex(key[idx], "^subnet", "" ) ) {
	  /* a little random problem here with allocation? */
	  menu[menusz++] = savestring(key[idx]);
	  menu[menusz-1] = as_rex(menu[menusz-1], ".*\\+", "", "g");
	  menu = xrealloc(menu, sizeof(menu) * (menusz + 1));
	  menu[menusz++] = savestring(get_aa(config, key[idx]));
	  menu = xrealloc(menu, sizeof(menu) * (menusz + 1));
#if defined( _DEBUG ) && !defined( _INFO )
	  printf("\n%s=%s", menu[menusz-2], menu[menusz-1]);
#endif
	}
      }
#ifdef _DEBUG
# ifndef _INFO
      printf("\n\nPress any key..."); getchar();
# endif
      (void) initscr();
#endif
      asprintf(&mesg, "Choose option to change:");
      if (dialog_vars.input_result) dialog_vars.input_result[0] = '\0';
      rok = dialog_menu(title,
			mesg,
			22, 72, 17,
			menusz / 2, menu);
      free(mesg);
      if ( rok == 0 ) {
	choosenkey = savestring(dialog_vars.input_result);
	choosenvalue = savestring(get_aa(config, choosenkey));
	asprintf(&mesg, "%s selected:", choosenkey);
	rok = dialog_inputbox(title,
			      mesg,
			      22, 72,
			      choosenvalue, 0);
	free(mesg);
	if ( rok == 0 ) {
	  put_aa(config, choosenkey, dialog_vars.input_result);
#ifdef _DEBUG
	} else {
	  endwin();
	  printf("Canceled.");
# ifndef _INFO
	  printf("\n\nPress any key..."); getchar();
# endif
	  (void) initscr();
#endif
	}
	free(choosenkey);
	free(choosenvalue);
	free_double_pointer(menu, menusz);
	free(key);
	goto startagain;
      } else {
#ifdef _DEBUG
	endwin();
	printf("Canceled.");
# ifndef _INFO
	printf("\n\nPress any key..."); getchar();
# endif
	(void) initscr();
#endif
	free_double_pointer(menu, menusz);
	free(key);
	goto startagain;
      }
    } else if ( m_rex(dialog_vars.input_result, "Save", "") ) {
      /* Save and exit */
      if ( save_dhcpd_config(DEFCONFIG, config, k_lim, key) == 0 ) {
	asprintf(&mesg,
		 "\nConfiguration file was saved successfully, do not forget "
		 "restart service to apply changes. If there is any problem "
		 "you can recover backup file any time.\n\n"
		 "Backup file format:\n\n%s-YYYYMMDD-HHMMSS\n\n"
		 "Service restart example:\n\n"
		 "root@example:~# service isc-dhcp-server restart\n",
		 DEFCONFIG);
	dialog_msgbox(title, mesg, 22, 72, true);
	free(mesg);
	endwin();
      } else {
	asprintf(&mesg,
		 "\nSomething went wrong, cannot save dhcpd.conf file. "
		 "Please review the error and try again later.\n\n"
		 "Error number: %i\n\nDescription:\n\n%s\n",
		 errno, strerror(errno));
	dialog_msgbox(title, mesg, 22, 72, true);
	free(mesg);
	free(key);
	goto startagain;
      }
    } else if ( m_rex(dialog_vars.input_result, "Restore", "") ) {
      /* Restore previous config */
      free_double_pointer(menu, menusz);
      menusz = 0;
      menu = lsbkdir_fast_menu(&menusz);
      asprintf(&mesg, "Choose backup file to recover by date:");
      if (dialog_vars.input_result) dialog_vars.input_result[0] = '\0';
      rok = dialog_menu(title,
			mesg,
			22, 72, 17,
			menusz / 2, menu);
      free(mesg);
      if ( rok == 0 ) {
	choosenkey = savestring(dialog_vars.input_result);
	choosenkey = as_rex(choosenkey, "^", DEFPATH, "");
	destroy_aa(config);
	config = get_dhcpd_config(choosenkey);
	free(choosenkey);
      }
      free_double_pointer(menu, menusz);
      free(key);
      goto startagain;
    }
  } else {
    endwin();
#ifdef _DEBUG
    printf("Canceled.\n");
#endif
  }
  free_double_pointer(menu, menusz);
  free(key);
  destroy_aa(config);
  exit (EXIT_SUCCESS);
}
