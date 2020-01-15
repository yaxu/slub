package Sandbox;

# (c) Alex McLean 2004/02/29

use strict;

use threads;
use threads::shared;

use Time::HiRes qw/ sleep time /;
#use Word;

#use Audio::Beep;
use Net::OSC;
#use Audio::OSC::Client;
use Net::OpenSoundControl::Client;
use Net::OpenSoundControl::Server;
use Math::Trig;
use IO::Socket;

use lib '/slub/clients';
#use Client;

#use Inline (C => 'DATA',
#            LIBS => '-llo',
#            DIRECTORY => '/music/new/inline/cache'
#           );

use Utils;

my $singleton;


##

sub _new {
    my ($pkg, $p) = @_;
    $p ||= {};
    my $self = bless($p, $pkg);
    $self->_init();
    $singleton = $self;
    return($self)
}

##

sub _init {
    my $self = shift;
    my $reload : shared;
    my $modification : shared;
    warn("initialising..\n");
    $self->{reload} = \$reload;
    $self->{modification} = \$modification;
    $self->_init_osc;
    $self->{bangs} = 0;
    $self->{buffer_time} = 0.05;
    $modification = 1;
    $self->init if $self->can('init');
}

##

sub _init_osc {
    my $self = shift;
    my $osc = 
      Net::OpenSoundControl::Client->new(Host => 'localhost', Port => 57120)
      or die "couldn't connect to supercollider";
    
    $self->{osc} = $osc;
}

##

sub new_thread {
    my $self = shift;

    #if ($ENV{SCTIME}) {
    threads->create(sub {$self->sc_event_loop} );
  #}
    #else {
    #threads->create(sub {$self->spread_event_loop} );
  #}
    
    #} elsif ($ENV{INTERNAL_CLOCK}) {
    #    threads->create(sub {$self->event_loop} );
    #} else {
    #threads->create(sub {$self->mux_event_loop} );
    #}   
    #else {
    #threads->create(sub {$self->tick_event_loop} );
    #}
}

##

sub mux_event_loop {
    my $self = shift;

    close(STDERR);
    open(STDERR, '>>/tmp/sandbox.err');
    close(STDOUT);
    close(STDIN);
    $self->{bangs} = 0;
    my $c = Client->new(port     => 6010,
                        on_clock => sub {$self->on_clock(@_)},
                        timer    => 'ext',
                        tpb      => ($ENV{TPB} || 1),
                       );
    
    $self->{samples} = [map {$c->init_samples($_)} $ENV{SAMPLES}];
    
    $self->{c} = $c;
    
    $c->event_loop;
}

##

sub tick_event_loop {
    my $self = shift;

    close(STDERR);
    open(STDERR, '>>/tmp/sandbox.err');
    close(STDOUT);
    close(STDIN);

    my $listen_handle = 
      IO::Socket::INET->new(Proto     => 'udp',
                            LocalPort  => 6010,
                           )
          or die "socket: $@";
    
    my $data;

    while ($listen_handle->read($data, 44)) {
        warn("got: " . length($data) . " bytes.\n");
    }
}

##

sub event_loop {
    my $self = shift;
    
    my ($start, $point, $ticks_per_minute, $ticks, @changes);

    $self->{_bpm} = ($self->{bpm} ||= 130);
    
    close(STDERR);
    open(STDERR, '>>/tmp/sandbox.err');
    close(STDOUT);
    close(STDIN);

    ($start, $ticks_per_minute, $ticks) = (time(), $self->{_bpm}, 0);
    
    $point = $start;
    
    my $bangs_per_tick = ($self->{bangs_per_tick} ||= 4);
    my $bangs_per_minute = $bangs_per_tick * $ticks_per_minute;
    my $bangs = 0;
    my $bangs_since_change = 0;
    $start = $point;

    while (1) {
        $self->{now} = $point;
        $self->_do_bang();
        
        # --
        # sleep till next bang
    
        $self->{bangs} = $bangs++;
        $bangs_since_change++;
    
        # Recalculate from the start each time so we don't collect errors
        my $bang_seconds = ($bangs_since_change / $bangs_per_minute) * 60;
        
        $point = $bang_seconds + $start;    
        
        my $sleep = $point - time();
        sleep($sleep) if $sleep > 0;
        
        if (($bangs % $bangs_per_tick) == 0) {
            $self->{ticks} = ++$ticks;
            $self->{total_ticks} = ++$ticks;
            if ($self->{bpm} != $self->{_bpm}) {
                $start = $point;
                $ticks_per_minute = $self->{_bpm} = $self->{bpm};
                $ticks = 0;
                $bangs_per_minute = $bangs_per_tick * $ticks_per_minute;
                $bangs_since_change = 0;
            }
        }
    }
}

##

sub spread_event_loop {
    #use Spread::Session;
    use YAML;
    
    my $self = shift;
    
    my $start_mod = 4;

    my ($start, $point, $ticks_per_minute, $ticks, $total_ticks, @changes);
    
    close(STDERR);
    open(STDERR, '>>/tmp/sandbox.err');
    close(STDOUT);
    close(STDIN);
warn("spreading");
    my $spread = $self->{spread} = 
      Spread::Session->new(MESSAGE_CALLBACK => 
                           sub {
                               my ($command, $p) = Load($_[0]->{BODY});
                               if ($command eq 'ticks_per_minute') {
                                   push(@changes, $p);
                                   @changes = 
                                     sort{$a->{ticks} <=> $b->{ticks}}
                                       @changes;
                               } else {
                                   my $func = "on__$command";
                                   if ($self->can($func)) {
                                       $self->$func($_[0], $p);
                                   }
                               }
                           },
                          )
          or die;
    
    
   warn("hm"); 
    $spread->publish('#tm#localhost', Dump('new_listener'));
warn("publish");
    while (not @changes) {
        $spread->poll();
        $spread->receive();
    }
    $spread->subscribe('ticks_per_minute');
    $spread->subscribe('share');

    my $change = shift @changes;
    ($start, $ticks_per_minute, $ticks, $total_ticks) = 
      ($change->{start},
       $change->{ticks_per_minute},
       $change->{ticks},
       $change->{total_ticks}
      );
    $self->{start} = $start;
    $self->{tpm} = $ticks_per_minute;
    my $time = time();
    if ($start >= $time) {
        sleep($start - $time);
    }
    
    $point = $start;
    
    my $started = 0;
    while (not $started) {
        if (($ticks % $start_mod) == 0) {
            $started = 1;
        }
    
        $ticks++;
        $total_ticks++;
        my $tick_seconds = ($ticks / $ticks_per_minute) * 60;
        $point = $tick_seconds + $start;
        my $sleep = $point - time();
        sleep($sleep) if $sleep > 0;
    }
    
    my $bangs_per_tick = ($self->{bangs_per_tick} ||= ($ENV{BPT} || 4));
    my $bangs_per_minute = $bangs_per_tick * $ticks_per_minute;
    my $bangs = 0;
    my $bangs_since_change = 0;
    $start = $point;

    while (1) {
        $self->{now} = $point;
        $self->_do_bang();
    
        if ($spread->poll()) {
            $spread->receive(1);
        }
    
        # --
        # sleep till next bang
    
        $self->{bangs} = $bangs++;
        $self->{bangs_since_change} = $bangs_since_change++;
    
        # Recalculate from the start each time so we don't collect errors
        my $bang_seconds = ($bangs_since_change / $bangs_per_minute) * 60;

        $point = $bang_seconds + $start;    

        my $sleep = $point - time();
        sleep($sleep) if $sleep > 0;
    
        if (($bangs % $bangs_per_tick) == 0) {
            $self->{ticks} = ++$ticks;
            $self->{total_ticks} = ++$total_ticks;

            if (@changes and ($changes[0]->{ticks} <= $ticks)) {
                my $change = shift @changes;
                if ($ticks != $change->{ticks}) {
                    warn("processed a ticks_per_minute change " 
                         . ($ticks - $change->{ticks}) 
                         . " ticks too late.\n"
                        );
                }

                $start = $point;
                $ticks_per_minute = $change->{ticks_per_minute};
		$self->{tpm} = $ticks_per_minute;
                $ticks = 0;
                $bangs_per_minute = $bangs_per_tick * $ticks_per_minute;
                $bangs_since_change = 0;
		$self->{start} = $start;
            }
        }
    }
}

##

sub sc_event_loop {
  use Netclock::Listener;
  my $self = shift;

  close(STDERR);
  open(STDERR, '>>/tmp/sandbox.err');
  close(STDOUT);
  open(STDOUT, '>>/tmp/sandbox.err');
  close(STDIN);
  
  my $tpb = $ENV{TPB} || 4;
  my $l = Netclock::Listener->new();
  print STDERR "hello\n";
  $l->clocked($tpb, 
	      sub {
		  $self->{bangs} = shift;
		  $self->_do_bang();
	      }
      );
}

##

sub spread_say {
    my ($self, $command, $p) = @_;
    
    $self->{spread}->publish('share', Dump($command, $p));
}

##

sub alex {
    my ($self, $message) = @_;
    my $osc = Net::OSC->new;
    $osc->server(hostname => '212.158.255.157', port => 8000);
    $osc->command('/change');
    $osc->data(['change']);
    $osc->send_message;
}

##
{
    my $vosc;
    sub vbang {
        my ($self) = @_;
        if (not $vosc) {
            $vosc = Net::OpenSoundControl::Client->new(Host => '127.0.0.1',
                                               Port => 6060
                                              );
        }
        $vosc->send(['/bang']);
    }
    sub vsite {
        my ($self, $value) = @_;
        if (not $vosc) {
	  $vosc = Net::OpenSoundControl::Client->new(Host => '127.0.0.1',
					  Port => 6060
					 );
        }
        $vosc->send(['/site', 'f', $value]);
    }
    sub vbar {
        my ($self, $value) = @_;
	$value ||= 4;
        if (not $vosc) {
	  $vosc = Net::OpenSoundControl::Client->new(Host => '127.0.0.1',
					  Port => 6060
					 );
        }
        $vosc->send(['/bar', 'i', $value]);
    }

}

##

sub v {
    my ($self, $p) = @_;

    my $x         = ($p->{x}         || 0.5);
    my $y         = ($p->{y}         || 0.5);
    my $speed     = ($p->{speed}     || 1);
    my $direction = ($p->{direction} || 0);
    my $width     = ($p->{width}     || 1);
    my $length    = ($p->{length}    || 2);
    my $volume    = ($p->{volume}    || 1);
    my $reverb    = ($p->{reverb}    || 30);
    my $image     = ($p->{image}     || "");
    my $curve     = ($p->{curve}     || 0);
    my $osc = $self->{vosc} ||=
        Net::OpenSoundControl::Client->new(Host => 'localhost',
                                           Port => 12000
                                          );

    $osc->send(
               ['/trigger',
                # x y
                'f', $x, 'f', $y,
                # speed
                'f', $speed,
                # direction
                'f', $direction,
                # volume
                'f', $volume,
                # width length
                'f', $width, 'f', $length,
		'i', $reverb,
		'f', $curve,
		's', $image,
               ]
              );
}


{
    my $s;
    sub dave_say {
	my ($self, $word) = @_;

	$s ||= IO::Socket::INET->new(PeerAddr => 'localhost',
				     PeerPort => 1234,
				     Proto    => 'tcp',
				     Reuse    => 1
				    );
	warn("couldn't open 1234 socket") unless $s;
	syswrite($s, $word . "\r\n")
	  or undef $s;
    }
}

{
    my $osc;
    my %expanded;
    sub t {
	my $self = $singleton;
	if ($_[0] eq $self) {
	    shift;
	}
	my $p;
	if (ref($_[0])) {
	    $p = shift;
	}
	else {
	    $p = {@_};
	}

        my $sample = $p->{sample};
        my $speed  = $p->{speed};
        my $shape  = $p->{shape};
        my $pan    = $p->{pan};
        my $pan_to = $p->{pan_to};
        my $volume = $p->{volume};
        my $offset = $p->{offset};
        my $envelope_name = $p->{envelope_name};
        my $anafeel_strength  = $p->{anafeel_strength} || 0;
        my $anafeel_frequency = $p->{anafeel_frequency} || 0;
        my $accellerate       = $p->{accellerate};
        my $vowel             = $p->{vowel};
        my $scale             = $p->{scale};
        my $loops             = $p->{loops};
        my $duration          = $p->{duration};
        my $delay             = $p->{delay};
        my $delay2            = $p->{delay2};
        my $cutoff            = $p->{cutoff};
        my $resonance         = $p->{resonance};
 
        $sample ||= "chin/tik1.wav";
        $speed  ||= 1;
        $shape  ||= 0;
        $pan    ||= 0;
        $accellerate ||= 0;
        $offset ||= ($self->{offset} || 0);
        $vowel  ||= 0;
        $scale  ||= '';
        $loops  ||= 1;
        $duration ||= 0;
        $delay  ||= 0;
	$delay2 ||= 0;
	$cutoff ||= 0;
	$resonance ||= 0;
        
        if (not defined $volume) {
            $volume = 0.5;
        }

        if (not defined $pan_to) {
            $pan_to = $pan;
        }
        
        if (not defined $envelope_name) {
            $envelope_name = "percussive";
        }
        
        # don't play a volume of 0
        return unless $volume;
        
        $sample = $sample;
        if ($sample =~ /^(.*\/)(\d+)$/) {
            if ($expanded{$sample}) {
                $sample = $expanded{$sample};
            }
            else {
                my ($dir, $number) = ($1, $2);
                my $samples = $self->{sample_cache}->{$sample};
                if (not $samples) {
                    opendir(DIR, "/slub/samples/" . $dir)
                        or return;
                    $samples = 
                        $self->{sample_cache}->{$sample} =
                        [grep {/\.[Ww][Aa][Vv]$/} 
                         sort readdir(DIR)
                        ];
                    
                    closedir(DIR);
                }
                return unless @$samples;
                my $origsample = $sample;
                $sample = $dir . $samples->[$number % @$samples];
                $expanded{$origsample} = $sample;
            }
            
        }
        
        if (not $osc) {
            $osc =
               Net::OpenSoundControl::Client->new(Host => 'localhost', Port => 7770)
      or die "couldn't connect to 7770";
        }

        my $message = 
		    ['/trigger',
		     's', $sample, 
		     'f', $speed, 
		     'f', $shape, 
		     'f', $pan,
		     'f', $pan_to,
		     'f', $volume,
		     's', $envelope_name,
		     'f', $anafeel_strength,
		     'f', $anafeel_frequency,
		     'f', $accellerate,
		     's', $vowel,
		     's', $scale,
		     'f', $loops,
		     'f', $duration,
		     'f', $delay,
		     'f', $delay2,
		     'f', $cutoff,
		     'f', $resonance
		    ];
	#if ($offset) {
	  $message = ['#bundle', 
		      $self->{now} + $offset,
		      $message
		     ];
      #}
	warn("offset: $offset, now: $self->{now}\n");


	$osc->send($message);
        #$osc->send_message($self->{now} + $self->{buffer_time} + $offset);
	#$self->{prev} = $self->{now};
     #   $osc->server(hostname => '111.112.113.1', port => 88000);
	#$osc->send_message();
    }
}

##

sub bpm {
    my ($self, $ticks_per_minute) = @_;
    
    $self->{spread}->publish('ticks_per_minute', 
                             Dump('ticks_per_minute', 
                                  {ticks_per_minute => $ticks_per_minute,
                                   ticks => $self->{ticks} + 2,
                                   total_ticks => $self->{total_ticks} + 2,
                                  }
                                 )
                            );
}

##

sub midi {
    my $self = shift;
    $self->{midi} ||= MIDI::Realtime->new;
    $self->{midi}->note(@_);
}

##

sub _do_bang {
    my $self = shift;
    print(STDERR "bang\n");
    if (__PACKAGE__->can('bang')) {
        eval {
            $self->bang();
        };
        if ($@) {
            print(STDERR $@);
            $self->reload(1);
        }
    }
    if ($self->reload) {
        if ($self->interpret) {
            $self->reload(0);
        }
    }
}

##

sub code {
    my $self = shift;
    if (not $self->{code}) {
        my @code;
        share(@code);
        $self->{code} = (\@code);
    }
    return($self->{code});
}

##

sub regex_e {
    my ($self, $regex, $replacement) = @_;
   
    return unless defined $replacement;
   
    my $code = join("\n", @{$self->code});
   
    eval "\$code =~ s/$regex/$replacement/seg";

    @{$self->code} = split("\n", $code);

}

##

sub regex {
    my ($self, $regex, $replacement) = @_;
    
    return unless defined $replacement;
    
    my $code = join("\n", @{$self->code});
    
    $code =~ s/$regex/$replacement/s;
    
    @{$self->code} = split("\n", $code);
}

##

sub modified {
    my $self = shift;
    $ {$self->{modification}}++;
    if ($ENV{DANGEROUS}) {
        $self->reload(1);
    }
}

##

sub trig {
    my ($self, $p) = @_;
    
    my ($sample, $pitch, $cutoff, $vol, $pan, $res) = 
      map{$p->{$_}} 
        qw{ s pitch cutoff vol pan res };
    $sample ||= $p->{sample}; 
    $pitch  = 60    unless defined $pitch;
    $cutoff = 20000 unless defined $cutoff;
    $vol    = 100   unless defined $vol;
    $pan    = 0.5 unless defined $pan;
    $res    = 0 unless defined $res;
    
    if ($sample !~ m,^/,) {
        $sample = '/yaxu/samples/' . $sample; 
    }
    
    if ($sample =~ m,^(.+?/)(\d+)$,) {
        my ($dir, $number) = ($1, $2);
        my $samples = $self->{sample_cache}->{$sample};
        if (not $samples) {
            opendir(DIR, $dir)
                or return;
            $samples = 
                $self->{sample_cache}->{$sample} =
                [grep {/\.[Ww][Aa][Vv]$/} 
                 readdir(DIR)
                 ];
            
            closedir(DIR);
        }
        return unless @$samples;
        $sample = $dir . $samples->[$number % @$samples];
    }

    if (not $self->{sample_lookup}->{$sample}++) {
        $self->{c}->send("MSG", "load $sample as $sample");
    }
    
    $self->{c}->play_sample($sample, $pitch, $cutoff, $vol, $pan, $res);
}

##

sub play {
    my $self = shift;
    my ($num, $gain, $pan, $formfreq, $bwfreq, $ts, $offset, $crackle, $browndel, $filter);
    
    if (ref($_[0])) {
        ($num, $gain, $pan, $formfreq, $bwfreq, 
         $ts, $offset, $crackle, $browndel, $filter) = 
           map {$_[0]->{$_}}
             qw{num gain pan formfreq bwfreq ts offset crackle browndel
                filter
               };
    } else {
        ($num, $formfreq, $bwfreq, 
         $ts, $offset, $pan, $gain, $crackle, $browndel, $filter) = @_;
    }
    
    $formfreq ||= 10;
    $bwfreq ||= 0;
    
    $pan = 0.5 if not defined $pan;

    $offset ||= 0;
    
    $num ||= 160;
    $browndel ||= 0;

    $filter ||= 44000;
    
    my $osc = $self->{osc};

    $gain ||= 100;
    my ($lgain, $rgain) = ($gain, $gain);
    if ($pan > .5) {
        $lgain *= (1 - (($pan - .5) * 2));
    } elsif ($pan < 0.5) {
        $rgain *= ($pan * 2);
    }
    $crackle ||= 10;
    $ts ||= 20;
    
    $osc->send(['#bundle', 
                $self->{now} + $self->{buffer_time} + $offset,
                ['/play',
                 's', 'on',
                 'i', $num, 
                 'i', $formfreq,
                 'i', $bwfreq,
                 'i', $ts,
                 'i', $lgain,
                 'i', $rgain,
                 'i', $crackle,
                 'i', $browndel,
                 'i', $filter
                ]
               ]
              );
}

sub sine {
    my $self = shift;
    my $p = shift;

    my $osc = $self->{osc};

    $p->{freq}      ||= 800;
    $p->{ts}        = exists($p->{ts})     ? $p->{ts}     : 1;
    $p->{volume}    = exists($p->{volume}) ? $p->{volume} : 1;
    $p->{direction} ||= 0;
    $p->{offset}    ||= 0;

    $p->{tri}       = ($p->{tri} or 0);
    $p->{saw}       = ($p->{saw} or 0);
    $p->{sin}       = ($p->{sin} or 0);

    if (not ($p->{tri} or $p->{saw} or $p->{sin})) {
	$p->{sin} = 1;
    }

    $p->{clip}     ||= 0;

    $osc->send(['#bundle', 
                $self->{now} + $self->{buffer_time} + $p->{offset},
                ['/nice',
                 'f', $p->{freq}, 
                 'f', $p->{ts},
                 'f', $p->{volume},
                 'f', $p->{direction},
		 'f', $p->{noise},
		 'f', $p->{sin},
		 'f', $p->{tri},
		 'f', $p->{saw},
		 'f', $p->{clip},
		 'f', ($p->{pan} || 0),
                ]
               ]
              );
}


sub noise {
    my $self = shift;
    my ($pan, $ts, $browndel, $filter, $gain, $offset, $envtype) = 
        map {$_[0]->{$_}}
        qw{pan ts browndel filter gain offset envtype};
    
    $pan = 0.5 if not defined $pan;
    $ts ||= 20;
    $browndel ||= 0;
    $filter ||= 44000;
    $gain ||= 100;
    $offset ||= 0;
    
    $envtype ||= 0;
    
    my ($lgain, $rgain) = ($gain, $gain);
    if ($pan > .5) {
        $lgain *= (1 - (($pan - .5) * 2));
    } elsif ($pan < 0.5) {
        $rgain *= ($pan * 2);
    }
    
    my $osc = $self->{osc};
    $osc->send(['#bundle', 
                $self->{now} + $self->{buffer_time} + $offset,
                ['/noise',
                 's', 'on',
                 'i', $lgain,
                 'i', $rgain,
                 'i', $ts,
                 'i', $browndel,
                 'i', $filter,
                 'i', $envtype
                ]
               ]
              );
}

sub trigger {
    my $self = shift;

    my ($sample, $gain, $offset, $pan, $crackle, $noise, $ts,
        $browndel, $rate, $env, $filter)
      = 
        map {$_[0]->{$_}}
          qw{ s gain offset pan crackle noise ts browndel rate env filter };
    
    $sample ||= $_[0]->{sample};
    
    $pan = 0.5 if not defined $pan;

    $offset ||= 0;

    $noise ||= 0;
    $browndel ||= 0;
    $rate ||= 100;
    $rate += 100;
    $env ||= 0;
    $filter ||= 0;
    return unless $sample;
    
    my $osc = $self->{osc};

    $gain ||= 100;
    my ($lgain, $rgain) = ($gain, $gain);
    if ($pan > .5) {
        $lgain *= (1 - (($pan - .5) * 2));
    } elsif ($pan < 0.5) {
        $rgain *= ($pan * 2);
    }
    $crackle ||= 10;
    $ts ||= 20;
    
    if ($sample !~ m,^/,) {
        $sample = '/yaxu/samples/' . $sample; 
    }

    if ($sample =~ m,^(.+?/)(\d+)$,) {
        my ($dir, $number) = ($1, $2);
        my $samples = $self->{sample_cache}->{$sample};
        if (not $samples) {
            opendir(DIR, $dir)
              or return;
            $samples = 
              $self->{sample_cache}->{$sample} =
                [grep {/\.[Ww][Aa][Vv]$/} 
                 readdir(DIR)
                ];

            closedir(DIR);
        }
        return unless @$samples;
        $sample = $dir . $samples->[$number % @$samples];
    }

    $osc->send(['#bundle',
                $self->{now} + $self->{buffer_time} + $offset,
                ['/trigger',
                 's', 'on',
                 's', $sample, 
                 'i', $lgain,
                 'i', $rgain,
                 'i', $crackle,
                 'i', $noise,
                 'i', $ts,
                 'i', $browndel,
                 'i', $rate,
                 'i', $filter,
                ]
               ]
              );
}

##

sub interpret {
    my $self = shift;
    my $result;
    my $code = join("\n", @{$self->code});
    no warnings 'redefine';
    #open(FH, ">>/tmp/sandbox_log");
    #print(FH $code);
    
    eval("package Sandbox::Test; use Math::Trig; $code");
    if ($@) {
        print(STDERR $@);
    } elsif (not Sandbox::Test->can('bang')) {
        eval("test failed - no bang!");
    } else {
        eval("package Sandbox; $code");
        $result = 1;
    }
    #close(FH);
    if ($self->can('on_interpret')) {
        $self->on_interpret;
    }
    return($result);
}

##

sub reload {
    return(@_ > 1 ? ($ {$_[0]->{reload}} = $_[1]) : $ {$_[0]->{reload}});
}

##

sub modification {
    return(@_ > 1 
           ? ($ {$_[0]->{modification}} = $_[1]) 
           : $ {$_[0]->{modification}}
          );
}

##

sub sleeptime {
    1
}

##

sub bangs_per_minute {
    160 * 4;
}

##

sub on_clock {
    my $self = shift;
    $self->{now} = $_[5];
    if (__PACKAGE__->can('bang')) {
        eval {
            $self->bang();
        };
        if ($@) {
            print(STDERR $@);
            $self->reload(1);
        }
    }
    if ($self->reload) {
        if ($self->interpret) {
            $self->reload(0);
        }
    }
    $self->{bangs}++;
    $self->{bangs};
}

##

sub init_samples {
    my ($self, $sampleset) = @_;
    my $sample_dir = "/slub/samples/$sampleset";

    opendir(DIR, $sample_dir) || die "can't opendir $sample_dir: $!";
    my @files = map {$sample_dir . '/' . $_} grep { -f "$sample_dir/$_" } readdir(DIR);
    closedir DIR;
    
    @{$self->{samples}} = grep {/\.wav$/} @files;
}

##

sub sin {
    my $self = shift;
    my $div = shift;
    $div ||= 1;
    ((CORE::sin($self->{bangs} / $div) + 1) / 2)
}

##

{
    my $meshosc;        


    sub strike {
	my ($self, $p) = @_;
	$meshosc ||= 
	    Net::OpenSoundControl::Client->new(Host => 'localhost', 
					       Port => 6011);
	$meshosc->send(['/strike',
			'f', $p->{x} || 0,
			'f', $p->{y} || 0,
			'f', $p->{velocity} || 1,
			'f', $p->{mass} || 0,
			'f', $p->{alpha} || 0
		       ]);
	
    }

    sub vector {
	my ($self, $p) = @_;
	$meshosc ||= 
	    Net::OpenSoundControl::Client->new(Host => 'localhost', 
					       Port => 6011);
	$meshosc->send(['/vector',
			'f', $p->{x} || 0,
			'f', $p->{y} || 0,
			'f', $p->{angle} || 0,
			'f', $p->{speed} || 0,
			'f', $p->{velocity} || 1,
			'f', $p->{mass} || 0,
			'f', $p->{alpha} || 0
		       ]);
	
    }

    sub tension {
	my ($self, $tension) = @_;
	$meshosc ||= 
	    Net::OpenSoundControl::Client->new(Host => 'localhost', 
					       Port => 6011);
	$meshosc->send(['/tension',
			'f', $tension || 1
		       ]);
    }
    sub loss {
	my ($self, $loss) = @_;
	$meshosc ||= 
	    Net::OpenSoundControl::Client->new(Host => 'localhost', 
					       Port => 6011);
	$meshosc->send(['/loss',
			'f', $loss || 1
		       ]);
    }

    sub strike_hi {
	my ($self, $p) = @_;
	$meshosc ||= 
	    Net::OpenSoundControl::Client->new(Host => 'localhost', 
					       Port => 6011);
	$meshosc->send(['/strike/hi',
			'f', $p->{x} || 0,
			'f', $p->{y} || 0,
			'f', $p->{velocity} || 1,
			'f', $p->{mass} || 0,
			'f', $p->{alpha} || 0
		       ]);
	
    }

    sub vector_hi {
	my ($self, $p) = @_;
	$meshosc ||= 
	    Net::OpenSoundControl::Client->new(Host => 'localhost', 
					       Port => 6011);
	$meshosc->send(['/vector/hi',
			'f', $p->{x} || 0,
			'f', $p->{y} || 0,
			'f', $p->{angle} || 0,
			'f', $p->{speed} || 0,
			'f', $p->{velocity} || 1,
			'f', $p->{mass} || 0,
			'f', $p->{alpha} || 0
		       ]);
	
    }

    sub tension_hi {
	my ($self, $tension) = @_;
	$meshosc ||= 
	    Net::OpenSoundControl::Client->new(Host => 'localhost', 
					       Port => 6011);
	$meshosc->send(['/tension/hi',
			'f', $tension || 1
		       ]);
    }
    sub loss_hi {
	my ($self, $loss) = @_;
	$meshosc ||= 
	    Net::OpenSoundControl::Client->new(Host => 'localhost', 
					       Port => 6011);
	$meshosc->send(['/loss/hi',
			'f', $loss || 1
		       ]);
    }

    sub strike_lo {
	my ($self, $p) = @_;
	$meshosc ||= 
	    Net::OpenSoundControl::Client->new(Host => 'localhost', 
					       Port => 6011);
	$meshosc->send(['/strike/lo',
			'f', $p->{x} || 0,
			'f', $p->{y} || 0,
			'f', $p->{velocity} || 1,
			'f', $p->{mass} || 0,
			'f', $p->{alpha} || 0
		       ]);
	
    }

    sub vector_lo {
	my ($self, $p) = @_;
	$meshosc ||= 
	    Net::OpenSoundControl::Client->new(Host => 'localhost', 
					       Port => 6011);
	$meshosc->send(['/vector/lo',
			'f', $p->{x} || 0,
			'f', $p->{y} || 0,
			'f', $p->{angle} || 0,
			'f', $p->{speed} || 0,
			'f', $p->{velocity} || 1,
			'f', $p->{mass} || 0,
			'f', $p->{alpha} || 0
		       ]);
	
    }

    sub tension_lo {
	my ($self, $tension) = @_;
	$meshosc ||= 
	    Net::OpenSoundControl::Client->new(Host => 'localhost', 
					       Port => 6011);
	$meshosc->send(['/tension/lo',
			'f', $tension || 1
		       ]);
    }
    sub loss_lo {
	my ($self, $loss) = @_;
	$meshosc ||= 
	    Net::OpenSoundControl::Client->new(Host => 'localhost', 
					       Port => 6011);
	$meshosc->send(['/loss/lo',
			'f', $loss || 1
		       ]);
    }
}

##

sub b {
    $singleton->{bangs}
}

sub say_hi {
    Word::play(1, $singleton, @_);
}

sub say_lo {
    Word::play(0, $singleton, @_);
}

sub say {
    Word::play(0, $singleton, @_);
}

##

1;

__DATA__
__C__

#include "lo/lo.h"

int send_osc(char *sample, double speed)
{
    lo_address t = lo_address_new(NULL, "7770");
    if (lo_send(t, "/trigger", "sf", sample, (float) speed) == -1) {
      printf("OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t));
    }
}
