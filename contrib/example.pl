#! /usr/bin/perl -w

sub on_xosd {
# $_[0] - signal name
# $_[1] - signal source
# $_[2] - signal destination
# $_[3] - message to pass to xosd
# Modify any of the above variables to change them.
# Below is an example of how to change text that is to be shown
  $_[3] =~ s/gg/GNU Gadu/g;
}

sub on_msg {
# $_[0] - signal name
# $_[1] - signal source
# $_[2] - signal destination
# $_[3] - user id that sent message
# $_[4] - message
# $_[5] - message class
# $_[6] - message time
# Modify any of the above variables to change them.
# Below is an example of how to change message that is to be displayed
  $_[4] =~ s/money/*MONEY*/g;
}

sub userlist_watch {
# $_[0] - protocol name
# $_[1] - action (see repo for more info)
# $_[2] - user id
# You can't modify above values.
  GGadu::signal_emit ("sound play file", "/usr/share/gg2/sounds/usr.wav", "sound*", 0);
}

# GGadu::register_script() must be called at the very begginning
GGadu::register_script ("example.pl");

# GGadu::signal_hook() places a hook on a given signal
GGadu::signal_hook ("example.pl", "xosd show message", "on_xosd");
GGadu::signal_hook ("example.pl", "gui msg receive", "on_msg");

# GGadu::repo_watch_userlist() places a hook on userlist changes
# Put "*" as a second argument to watch every protocol or give a protocol
# name to watch only one protocol
GGadu::repo_watch_userlist ("example.pl", "*", "userlist_watch");

