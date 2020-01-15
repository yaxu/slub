use MIDI::Music;
use Fcntl;
my $mm = new MIDI::Music;

# Initialize device for writing
$mm->init('mode'     => O_WRONLY,
    'timebase' => 96,
    'tempo'    => 60,
    'timesig'  => [2, 2, 24, 8],
    ) || die $mm->errstr;

# Play a C-major chord
$mm->playevents([#['patch_change', 0, 0, 49],
    ['note_on', 0, 0, 60, 64],
    ['note_on', 0, 0, 64, 64],
    ['note_on', 0, 0, 67, 64],
    ['note_on', 0, 0, 72, 64],
    ['note_off', 144, 0, 60, 64],
    ['note_off', 0, 0, 64, 64],
    ['note_off', 0, 0, 67, 64],
    ['note_off', 0, 0, 72, 64],
    ]) || die $mm->errstr;
my $foo = <>;
$mm->dumpbuf;
$mm->close;

