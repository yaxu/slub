#####################################
# few functions to replace the dumpOSC utility
# in perl directly
use vars qw(@ISA @EXPORT);
use Exporter;

@ISA = qw(Exporter);
@EXPORT = qw(make_sendargv ParseOSC enter_channel read_chat);

# return 1;

# below should be in sendOSC actually!!!!

###########################
# generate and return fake argument vector for call to sendOSC function
#sub make_sendargv() {
#  my ($host,$port) = @_;
#  my @argv = ();
#  # push @argv,$0,"-notypetags","-h",$host,$port;
#  push @argv,$0,"-h",$host,$port;
##  print $#argv,"\n";
#  return @argv;
#}

###########################################
# function 1 for parsing OSC packetz
# checks for bundle, calls itself recursively and when message found, calls
# DataAfterAlignedString which ought be called something else
sub ParseOSC ($$$) {
  my ($msg,$msglen,$sock) = @_;
  my @msgs;
  my @msgary = split "",$msg; # split message bytes into array
  my $debug = 0;

  if($debug){print "="x60,"\n";}
  if($debug){
    print "ParseOSC: msg:       '$msg'\n";
    print "ParseOSC: msglen:    '$msglen'\n";
    print "ParseOSC: msgarylen: ",$#msgary + 1,"\n";
  }
  # print "ParseOSC: socket: '$sock'\n";

  my $mai = 0;
  my $size = 0;

  my %data;
  
  if(($msglen%4)!=0) {
    print "ParseOSC: bad msg: length not 4 byte aligned.\n";
    return;
  }

  my $string="#bundle";
  if (($msglen >= 8) && ($msg =~ m/^\$string/)) {
    ### this is a bundle message
    print "ParseOSC: bundle msg\n";

    #message: 8 bytes bundle, 4 bytes + 4bytes timecode, arbi bytes messages
    ###### bundle
    $mai += 8; # skip bundle
    my $bundle = unpack("a8",substr($msg,0,$mai));
    print "ParseOSC: bundle: $bundle\n";

    if ($msglen < 16) {
      printf("ParseOSC: bundle msg too small (%d bytes) for time tag\n",$msglen);
      return;
    }

    ###### timetag
    $mai += 8; # skip timetag?
    my $tt1 = unpack("N",substr($msg,8,4));
    my $tt2 = unpack("N",substr($msg,12,4));
    printf("timetag: %lx%08lx\n",$tt1, $tt2);
    printf("[ %lx%08lx\n", $msgary[8],$msgary[12]);

    ###### data
    # $mai
    for(;$mai<$msglen;$mai+=4) {
      print"mai: $mai\n";
      $size = unpack("N",substr($msg,$mai,$mai+4));
      $mai += 4;
      print "size: $size\n";

      if(($size % 4) != 0) {
	printf("bad size %d in bundle\n",$size);
	return;
      }
      if(($size + $mai + 4) > $msglen) {
	printf("bad size %d in bundle, only %d left;\n",$size,$msglen-$mai-4);
	return;
      }

      $mai += $size;
      &ParseOSC(substr($msg,$mai-$size,$mai), $size, $sock);

      #$mai = $mai + 4;
    }
    print "]\n";
  } else {
    my $msgname =  unpack("a$msglen",$msg);
    #    my $msgname =  $msg;
    %data = &DataAfterAlignedString($msgname,$msglen);
#    print "ParseOSC: data: ".$data{addr}." ".$data{vals}."\n";
  }
#  $msgs[0] = ('addr' => '/bla',
#	      'vals' => '1,2,3.444,"bla"');
#  $msgs[1] = ('addr' => '/bli',
#	      'vals' => '4,5,6.777,"bli"');
#  $msgs[0] = \%data;
#  return ('bundle' => 0,
#	  'timestamp' => 00000001,
#	  'data' => %data);
  return %data;
}

################################################
# eats the message and returns address and formatted parameter list
sub DataAfterAlignedString {
  ### effectively ParseOSCSingle message
  my $debug = 0;
  my ($msg,$len) = @_;
  my @msg = split //,$msg;
#  print "msg: ",join //,@msg,", len: $len\n";
  my @msgname;
  my $msgname = "";
  my $i;

  my $typestring;
  my $typestringc;
  my @types;

  my $s;
  my $cs;

  my $ret;

  my $int;
  my $f;

  for($i=0;;$i++){
    if($msg[$i] eq "\0") { $i++;last; }
    else { push @msgname,$msg[$i]; }
  }
  $msgname = join("",@msgname);
#  $ret .= "$msgname ";

  #  foreach my $bl (@msgname) {
  #    print "$bl","\n";
  #  }
  # why blank space here in join?
  if($debug){print "DAAS: msgname: ",$msgname,"\n";}

  # check for the last \0
  for(; ($i % 4) != 0;$i++){
    # print "skip 0's",$msg[$i],"\n";
  }

  ########################
  # $i now points to the first not \0 after padding after messagename
  # now go thru the arguments 32 bitz at a time
  #  print "len: $len\n";

  # now we should check for typetag-string
  # if we have a comma: ',' its a type tag strng
  if($msg[$i] eq ",") {
    if($msg[$i+1] ne ",") {
      if($debug){print "DAAS: typetags string jodel!\n";}
      ($typestringc,$typestring) = &get_nextstring($i,@msg);
      $i += $typestringc;
      $typestring  = substr($typestring,1);
 #     $typestringc = length($typestring);
      if($debug){print "DAAS: typestring: '$typestring', count (incl 0 padding): $typestringc\n";}
      @types = split //,$typestring;
      $typei = 0;
      foreach $type (@types) {
#	print $type,"\n";
      }
    }
  } else {
    # better or worse? uncomment and prepend 1
    # $i+=4;
  }
  # XXX: dirty + unknown

  # $i--;

  for (;$i<$len;) {

    # unpack with type information
    if($#types > -1) { # types array is longer than 0 so we have a typestring
      if($types[$typei] eq "i") { # integer
	# XXX: N must be different here, namely signed integer?
	# $int = unpack("N",&get_next4bytez($i,@msg));
	$int = unpack("N",&get_next4bytez($i,@msg));
	# XXX: since i cant get unpack to do it right with the
	# supposedly signed big-endian longs.
	if ($int > 2147483648) {
	  $int = (4294967296 - $int) * -1;
	}
	$ret .= "$int,";
	$i+=4;
      } elsif($types[$typei] eq "f") { # float
	$f = unpack("f",reverse(&get_next4bytez($i,@msg)));
	$ret .= "$f,";
	$i+=4;
      } elsif($types[$typei] eq "s") { # string
	# my $debug = 0;
	($cs,$s) = &get_nextstring($i,@msg);
	# next if($cs == 0);
	if($debug){print STDERR "DAAS: string: '$s' sowie count: $cs\n";}
	$i += $cs;
	$cs = 0;
	$ret .= "$s,";
      } else {
	print STDERR "dumpOSC_util: DAAS: unrecognized typetag '", $types[$typei], "'\n";
	$i+=4;
      }
      # increment types index
      $typei++;
    #################################
    # untyped "guessed" types ...
    # this code is pretty wobbly
    } else {
      # print "argcount: $i, len: $len\n";

      # get next four bytes from array with offset x
      $s = &get_next4bytez($i,@msg);

      # make int from these 4 bytes
      $int = unpack("N",$s);
      # and float, reverse the characterstring priorily ..
      $f   = unpack("f",reverse($s));
      
      # it is an int with these boundaries
      if($int >= -1000 && $int <= 1000000) {
	#      printf("int: '%d'\n",$int);
	$ret .= "$int,";
      }
      #    elsif ($f >= -1000.0 && $f <= 1000000.0 &&
      #	   $f <= 0.0 || $f >= 0.000001) { # smallest opsitive float
      # now its a float, checking might be XXX
      elsif ($f >= -1000.0 && $f <= 1000000.0) {
	#      printf("flt: '%f'\n",$f);
	$ret .= "$f,";
      }
      # its a string. need to see wether there's string in the next 4 bytes
      else {
	#      printf("str: '%s'\n",$s);
	($cs,$s) = &get_nextstring($i,@msg);
	#      print "count: $cs, stuff: $s\n";
	#      $s .= $ts;
	$i += $cs;
	$cs = 0;
	#      printf("str: '%s'\n",$s);
	$ret .= "$s,";
      }
      $i+=4;
    }
    # $i+=4;
    $s = "";
    if($debug){print "DAAS: i: $i\n";}
  }
  $i = 0;
  chop($ret)
    if $ret;
  return ('addr' => $msgname,
	  'vals' => $ret);
}

############################
# helper func for osc parsing, get the next four bytes of packet/array
sub get_next4bytez () {
  my ($offs, @q) = @_;
  my $s;
  for($qu=0;$qu<4;$qu++) {  # && $msg[$i+$qu] != '\0'
    #      print "innerloop: qu: $qu, ".$msg[$i+$qu]."\n";
    $s .= $q[$offs+$qu];
  }
  return $s;
}

############################
# pretty much like get_next4bytez but gets a whole string
# searching for terminating null chars
sub get_nextstring () {
  my ($offs,@msg) = @_;
  # my @msg = split //,$msg;
  my $j;
  my @retstr;
  my $debug = 0;
  # print "try\n";
  for ($j=0;;$j++){
    # if($msg[$offs+$j] eq '\0' || $msg[$offs+$j] eq '') {last;}
    if(ord($msg[$offs+$j]) == 0){$j++;last;}
    else {push @retstr,$msg[$offs+$j];}
    if($debug){
      printf("g_ns:  qu: %s - %d\n",$msg[$offs+$j],$j);
      print  "g_ns:  qu: ", ord($msg[$offs+$j]),"\n";
    }
  }
  
  for(; ($j % 4) != 0;$j++){
    if($debug){ print "g_ns: skip 0's",$msg[$j],"\n"; }
  }
  if($debug){ print "g_ns: incount: $j\n"; }
  return ($j,join("",@retstr));
}

1;
