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
# Note2 : This version is quiet more verbose with errors and warnings.
#
# $Id: lua2dox.awk,v 1.2 2003-03-26 23:00:04 ben Exp $
#

function perror(text)
{
  print FNR ":" text > "/dev/stderr";
}

function pdebug(text)
{
  print "DEBUG:" FNR ":" text > "/dev/stderr";
}


# Add a warning.
function add_warning(text, fragment)
{
  warnings ++;
  text = FNR ":" text;
  if (warningbuf != 0) {
    warningbuf = warningbuf "\n" text;
  } else {
    warningbuf = text;
  }
  if (fragment) {
    warningbuf = warningbuf "\n>> " fragment;
  }
}

function change_mode (newmode, line,      pos, tag)
{
  # change_mode must be call once per line. This will check it.
  if (linemode) {
    linemode = linemode ":" newmode;
    add_warning("many modes for one line :" linemode, line);
  } else {
    linemode = newmode;
  }
    
  nparse++;
  if (newmode == "DOCUM") {
    # Convert the '---' to '///'
    gsub("^[[:blank:]]*---","///",line);

  } else if (newmode == "FUNCT") {
    pos = index(line, ")");
    if (pos) {
      line = substr(line,1,pos);
    }
    
  }

  if (newmode == curmode) {
    if (curmode != "BLANK") {
      if (curmode == "FUNCT") {
	buffer = buffer line;
      } else {
	buffer = buffer "\n" line;
#	buffer = buffer "\n" curmode ">" line;
      }
    }
  } else {
    if (curmode == "FUNCT") {
      if (funcname != 0) {
	add_warning("Unfinished function " funcname " (" newmode ")",$0);
	funcname = 0;
	buffer = "";
      } else {
	gsub("[[:space:]]","",buffer);
	gsub("[[:alnum:]_]+[,)]"," var &",buffer);
	buffer = buffer ";"
      }
    }
    curmode = newmode;
    print buffer;
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
  linemode = 0;
  funcname = 0;
  codestartline = 0
  discarded = 0;
}

/^[[:blank:]]*---/ {
# discarded bit 0:discard at final step, bit1:discarded here 
  pos = 0
  if (curmode == "FUNCT" && funcname != 0) {
    add_warning("Found a --- documentation line within function " funcname,$0);
    discarded = 3;
  } else {
    ;# Get doxygen tags (only gandle the @tag syntax
    pos = match($0,"^---[[:space:]]*@([[:alpha:]]+)",tag);
  }

  ;# Got a tag, is it a code inline tag ?
  if (pos) {
    if (codetags["end" tag[1]]) {
      ;# Perform some check for unmatched @tag - @endtag sequences
      if (code) {
	add_warning("Found a @" tag[1] " within a @" code " session. Discard @" code ".");
      }
      code = tag[1];
      discarded = 1;
      codestartline = FNR;
    } else if (codetags[tag[1]]) {
      ;# Got a tag, is it a end code inline tag ?
      if (code == 0) {
	add_warning("Found not matched @" tag[1] ". Discarded.");
	discarded = 3;
      } else if ("end" code != tag[1]) {
	add_warning("Found not matched @" tag[1] " within a @" code " session. Discarded.");
	discarded = 3;
      } else {
	code = 0;
	discarded = 1;
      }
    } else {
      ;# tag is not a code tag, if code mode discard line
      if (code != 0) {
	discarded = 1;
      }
    }
  } else if (code != 0) {
    ; # no tag, code mode, discard
    discarded = 1;
  }


  if (discarded > 1) { 
    discarded -= 2;
  } else {
    change_mode("DOCUM", $0);
  }
}

/^--:/ {
  if (code != 0) {
    add_warning("Found a --: documentation line within @" code " session",$0);
  } else if (curmode == "FUNCT") {
    add_warning("Found a --: documentation line within function " funcname,$0);
    discarded = 1;
  } else {
    change_mode("DIREC", substr($0,4));
  }
}

/^[[:cntrl:][:space:]]*-[^-:]/ {
# REM are considerate as BLANK
# Note that a single '-' is tested, since we only care about
# function declaration, and in that case no line should begin with a '-'
#     change_mode("REMAR", $0);
  if (code != 0) {
    if (curmode == "FUNCT") {
      add_warning("Comment inside function definition",$0);
      discarded = 1;
    } else {
      change_mode("BLANK","");
    }
  }
}

# Match blank line
/^[[:cntrl:][:space:]]*$/ {
  if (code == 0) {
    change_mode(curmode == "FUNCT" ? "FUNCT" : "BLANK","");
  }
}

# Match the function line
/^[[:blank:]]*function[[:blank:]]+[[:alnum:]_]+.*/ {
  if (code == 0) {
    if (!match($0,".*function[[:blank:]]+([[:alnum:]_]+)(.*)",
	       funcname2)) {
      add_warning("Internal : missing function name in\n" $0);
      change_mode("BLANK","");
    } else {
      if (funcname) {
	add_warning("Found new function " funcname2[1] " within " funcname);
      }
      funcname = funcname2[1];
      funcfound = 1;
    }
  } else {
#pdebug "documenting function directly with @" code ".\n" $0;
  }
}

{
  if (discarded) {
    discarded = 0;
#     pdebug(FNR ": REALLY DISCARD THIS LINE \n" $0);
  } else if (funcfound) {
    ;# Entering function :
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
      gsub("--.*","");
      change_mode("FUNCT",$0);
    }
  } else if (code != 0) {
    change_mode("DOCUM", "---" $0);
  }

  if (linemode == 0) {
    nlost ++;
  }
  linemode = 0;
  nlines ++;
}

END {
  if (code) {
    add_warning("Unfinished: " codestartline ":@" code " session.");
  }

  change_mode("ZEEND","");
  if ( warnings > 0 ) {
    print warnings " warnings:" > "/dev/stderr";
    print warningbuf > "/dev/stderr";
  }

  if (nlines != nparse) {
    fflush();
    perror("lines:" nlines " parsed:" nparse " lost:" nlost "-" nlines-nparse); 
#     exit 255;
  }
  exit 0
}
