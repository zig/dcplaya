# lua2dox version 3 : the return of awk.
#
# by benjamin gerard
#
# Fill free to use, modify this script.
#
# Please send any enhancement / debugging / comments to <ben@sashipa.com>
#
# Note : I am sure it can be done more efficiently but it is my very
#        first awk program :P. Anyway it is about 1.5 times faster
#        than my previous sed script and 3.6 times faster than the oldest
#        sh script.
#
# $Id: lua2dox.awk,v 1.1 2003-03-25 09:25:03 ben Exp $
#

function add_warning(text)
{
  warnings ++;
  text = FNR ":" text;
  if (warningbuf != 0) {
    warningbuf = warningbuf "\n" text;
  } else {
    warningbuf = text;
  }
}

function change_mode (newmode, line,      pos, tag)
{
  if (linemode) {
    linemode = linemode ":" newmode;
    add_warning("many modes for one line :" linemode);
  } else {
    linmode = newmode;
  }
    
  nparse++;
  if (newmode == "DOCUM") {
#    pos = index(line,"-");
#    line = substr(line,pos);
    gsub("^[[:blank:]]*---","///",line);
    
    pos = match(line,"^///[[:space:]]*@([[:alpha:]]+)",tag);
    if (pos) {
#      print "TAG ",tag[1],"found at",pos;
      if (codetags["end" tag[1]]) {
# defined start code tag
	if (code) {
	  add_warning("Found a @" tag[1] " within a @" code " session. Discard @" code ".");
	}
	code = tag[1]
      } else if (codetags[tag[1]]) {
# defined end code tag
	if (code == 0) {
	  add_warning("Found not matched @" tag[1] ". Discarded.");
	} else if ("end" code != tag[1]) {
	  add_warning("Found not matched @" tag[1] " within a @" code " session. Discarded.");
	} else {
	  code = 0;
	}
      }
    }
  } else if (newmode == "FUNCT") {
    pos = index(line, ")");
    if (pos) {
      line = substr(line,1,pos);
    }
    
  }

  if (newmode == curmode) {
#     print "ENQUEUE" line;
    if (curmode != "BLANK") {
      if (curmode == "FUNCT") {
	buffer = buffer line;
      } else {
	buffer = buffer "\n" line;
# curmode ">" line;
      }
    }
  } else {
#     print "Change mode " curmode "->" newmode;
    if (curmode == "FUNCT") {
      if (funcname != 0) {
	add_warning("Unfinished function " funcname);
	funcname = 0;
	buffer = "";
      } else {
	gsub("[[:space:]]","",buffer);
      }
    }
    curmode = newmode;
    print buffer; # "<<";
#    buffer = ">>" curmode ">" line;
    buffer = line;
  }

  if (pos && newmode == "FUNCT") {
# finish the function here
    funcname = 0;
  }

}

BEGIN {
  buffer = "";
  line = "";
  curmode="BEGIN";
  nlines = 0;
  nparse = -1; # Becoz of buffering
  nlost  = 0;
  codetags["endcode"] = 1;
  codetags["endverbatim"] = 1;
  code   = 0; # 0:no code, "value":in codetag mode @value
  warningbuf = 0;
  warnings = 0;
  errors = 0;
  linmode = 0;
  funcname = 0;
}

/^[[:blank:]]*---/ {
  change_mode("DOCUM", $0);
}

/^--:/ {
    change_mode("DIREC", substr($0,4));
}

/^[[:cntrl:][:space:]]*-[^-:]/ {
# REM are considerate as BLANK
# Note that a single '-' is tested, since we only care about
# function declaration, and in that case no line should begin with a '-'
#     change_mode("REMAR", $0);
  change_mode(curmode == "FUNCT" ? "FUNCT" : "BLANK","");
}

# Match blank line
/^[[:cntrl:][:space:]]*$/ {
  change_mode(curmode == "FUNCT" ? "FUNCT" : "BLANK","");
}

# Match the function line
/^[[:blank:]]*function[[:blank:]]+[[:alnum:]_]+.*/ {
  if (!match($0,".*function[[:blank:]]+([[:alnum:]_]+)(.*)",
	     funcname2)) {
    add_warning("Internal : missing function name in\n" $0);
    change_mode("BLANK","");
  } else {
    if (funcname) {
      add_warning("Found new function " funcname2[1] " within " funcname);
    }
#    gsub("--.*","",funcname2[2]);
    funcname = funcname2[1];
    funcfound = 1
#    change_mode("FUNCT",funcname " " funcname2[2]);
  }
}

{
  if (funcfound) {
# Entering function :
    funcfound = 0;
    gsub("--.*","");
    pos = index($0,"(");
    if (pos) {
      change_mode("FUNCT",funcname substr($0,pos));
    } else {
      change_mode("FUNCT",funcname);
    }
  } else if (curmode == "FUNCT") {
    if (funcname == 0) {
      change_mode("BLANK","");
    } else {
# Continue function
#      print("continue function " funcname " with " $0);
      gsub("--.*","");
      change_mode("FUNCT",$0);
    }
  }

  if (linmode == 0) {
    nlost ++;
  }
  linmode = 0;
  nlines ++;
}

END {
  change_mode("ZEEND","");
  if ( warnings > 0 ) {
    print warnings " warnings:";
    print warningbuf;
  }

#   if (nlines != nparse) {
#     fflush();
#     print "error lines:",nlines,"parsed:",nparse, "lost:",nlost,nlines-nparse > "/dev/stderr"; 
#     exit 255;
#   }
  exit 0
}
