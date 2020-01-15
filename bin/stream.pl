#!/usr/bin/perl -w
use Shout;
use strict;

my $conn = new Shout;

$conn->host('yaxu.org');
$conn->port(8000);
$conn->mount('yaxu');
$conn->password('');
$conn->name('yaxu');
$conn->url('http://yaxu.org/');
$conn->genre('perl');
$conn->format(SHOUT_FORMAT_MP3);
$conn->protocol(SHOUT_PROTOCOL_HTTP);
$conn->description('yaxu');
$conn->public(0);

### Set your stream audio parameters for YP if you want
$conn->set_audio_info(SHOUT_AI_BITRATE => 128, SHOUT_AI_SAMPLERATE => 44100);

### Connect to the server
$conn->open or die "Failed to open: ", $conn->get_error;

### Set stream info
$conn->set_metadata('song' => 'Yaxu Paxo');

### Stream some data
my ( $buffer, $bytes ) = ( '', 0 );
while( ($bytes = sysread( STDIN, $buffer, 4096 )) > 0 ) {
    warn("sending $bytes\n");
    $conn->send( $buffer );
    $conn->sync
}
  
### Now close the connection
$conn->close;
