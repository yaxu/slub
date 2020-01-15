use MIDI::Realtime;

         my $midi = MIDI::Realtime->new();

         # Play note 47 with maximum velocity on channel 1
         $midi->note(47, 127, 1);

         # Now have some fun with randomness

         my @notes      = (10 .. 60);
         # use all the channels (with extra drums)
         my @channels   = (1, 2);
         my @velocities = (10 .. 127);

         while (1) {
           $midi->balance(int rand(127));
           my $rand = rand(@notes);
           $midi->note($notes[$rand],
                       $channels[rand(@channels)],
                       $velocities[rand(@velocities)]
                      );

           # Wait for a tenth of a second
           select(undef,undef,undef, 0.10);
#           $midi->note($notes[$rand], 0, 0); 
         }

