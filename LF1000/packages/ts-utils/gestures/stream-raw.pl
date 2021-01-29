#!/usr/bin/perl

# Raw data:
# X (0-480) is scaled by 0.75 (ADC: 200-840)
# Y (0-272) is scaled by 0.6  (ADC: 290-740)

sub median (@)
{
    my @a = @_;
    return (sort {$a <=> $b} @a)[(scalar @a)/2];
}

sub dot(\@\@)
{
    my ($ra,$rb) = @_;
    my @a = @{$ra};
    my @b = @{$rb};
    my $s = 0;
    for (0..$#a)
    {
	$s += $a[$_] * $b[$_];
    }
    $s;
}

sub magnitude2 (@)
{
    my @a = @_;
    my $s = 0;
    $s += $_*$_ for (@a);
    $s;
}

sub magnitude (@)
{
    sqrt(magnitude2(@_));
}

@MOVING_STATES = qw{
    NONE
    STILL
    LINEAR
    OTHER
};

@GESTURE_STATES = qw{
    UNKNOWN
    IN_BANG
    IN_TOUCH
    IN_HOLD
    IN_SWIPE
};

@GESTURES = qw{
    NONE
    TOUCH
    HOLD
    GRIND
    BANG
    DRAG
    SWIPE
    FLICK
    SCRUB
    LASSO
};

use constant MAX_TNT_DOWN	=> 100;
use constant MIN_TNT_UP		=> 400;

use constant TNT_BAND_DOWN	=> 0;
use constant TNT_BAND_TRANS	=> 1;
use constant TNT_BAND_UP	=> 2;

@NAME_TNT_BAND = ("Up", "Tran", "Down");
@NAME_PEN_STATE = ("U", "V", "D", "^");

use constant PEN_STATE_UP	=> 0;
use constant PEN_STATE_GOING_DOWN	=> 1;
use constant PEN_STATE_DOWN	=> 2;
use constant PEN_STATE_GOING_UP	=> 3;

@NAME_PEN_ACTION = qw{none end start};

use constant PEN_ACTION_NONE	=> 0;
use constant PEN_ACTION_END	=> 1;
use constant PEN_ACTION_START	=> 2;

@NEXT_PEN_STATE = ( # [state][band] -> next state
    #      DOWN             TRANS             UP
    [ PEN_STATE_DOWN, PEN_STATE_GOING_DOWN, PEN_STATE_UP ], # PEN_STATE_UP
    [ PEN_STATE_DOWN, PEN_STATE_GOING_DOWN, PEN_STATE_UP ], # PEN_STATE_GOING_DOWN
    [ PEN_STATE_DOWN, PEN_STATE_GOING_UP,   PEN_STATE_UP ], # PEN_STATE_DOWN
    [ PEN_STATE_DOWN, PEN_STATE_GOING_UP,   PEN_STATE_UP ], # PEN_STATE_GOING_UP
);

@PEN_ACTION = ( # [state][band] -> ACTION
    #      DOWN             TRANS             UP
    [ PEN_ACTION_START, PEN_ACTION_NONE, PEN_ACTION_NONE ], # PEN_STATE_UP
    [ PEN_ACTION_START, PEN_ACTION_NONE, PEN_ACTION_NONE ], # PEN_STATE_GOING_DOWN
    [ PEN_ACTION_NONE,  PEN_ACTION_NONE, PEN_ACTION_END  ], # PEN_STATE_DOWN
    [ PEN_ACTION_NONE,  PEN_ACTION_NONE, PEN_ACTION_END  ], # PEN_STATE_GOING_UP
);

@NAME_MOVE_STATE = ("Still", "Change", "Moving");

use constant MOVE_STATE_STILL	=> 0;
use constant MOVE_STATE_CHANGE	=> 1;
use constant MOVE_STATE_MOVING	=> 2;

while (<>)
{
    next unless ($x, $y, $t, $p) = /([ 0-9]{4}) ([ 0-9]{4}) ([ 0-9]{4}) ([ 0-9]{4})/;

    # Decide TNT band
    $last_tnt_band = $tnt_band;
    $tnt_band = TNT_BAND_UP;
    $tnt_band = TNT_BAND_TRANS if $t < MIN_TNT_UP;
    $tnt_band = TNT_BAND_DOWN  if $t < MAX_TNT_DOWN;

    $pen_action = $PEN_ACTION[$pen_state][$tnt_band];
    $pen_state  = $NEXT_PEN_STATE[$pen_state][$tnt_band];

    printf ("%4d %4d %4d %4d band=%4s act=%5s state=%1s ", $x, $y, $t, $p,
	    $NAME_TNT_BAND[$tnt_band],
	    $NAME_PEN_ACTION[$pen_action],
	    $NAME_PEN_STATE[$pen_state]);

    if ($pen_action == PEN_ACTION_START)
    {
	# Prepare
	@x = @y = @t = @p = ();
	$gesture_state = NONE;
	$lmoving = $moving = 0;
	$moves = 0;
	$arc_len = 0;
	$mov_len = 0;
	$msx = $x;
	$msy = $y;
    }
    elsif ($pen_action == PEN_ACTION_END)
    {
	# Evaluate ending
	evaluate_end ();
	# Clean up
	@x = @y = @t = @p = ();
	$gesture_state = NONE;
	$lmoving = $moving = 0;
	$moves = 0;
    }
    elsif ($pen_state == PEN_STATE_DOWN)
    {
	# Accumulate
	push @x, $x; push @y, $y; push @t, $t; push @p, $p;
	# Evaluate
	evaluate ();
    }

    print "\n";
    ($lx, $ly, $lt, $lp) = ($x, $y, $t, $p);
}

sub evaluate ()
{
    $len = @x;
    if ($len < 10)
    {
	print "banging  ";
	$gesture_state = IN_BANG;
    }
    elsif ($len < 75)
    {
	print "touching ";
	$gesture_state = IN_TOUCH;
    }
    else 
    {
	print "holding  ";
	$gesture_state = IN_HOLD;
    }

    # Motion
    $m2xy = $mxy = $m2sxy = $msxy = $dot = 0;
    @dxy = ($x - $lx, $y - $ly);
    $m2xy = magnitude2 (@dxy);
    $mxy = sqrt($m2xy);
    @dsxy = ($x - $msx, $y - $msy);
    $m2sxy = magnitude2 (@dsxy);
    $msxy = sqrt($m2sxy);
    $mov_len = $msxy;

use constant THRESH_MOVING => 10;
    $moving = $m2xy > THRESH_MOVING;

    # Colinear?
    if ($moving)
    {
	$moves++;
	$m2 = $m2xy * $m2sxy;
	if ($m2 > THRESH_MOVING)
	{
	    $dot = dot (@dsxy, @dxy) / sqrt ($m2);
	}
	$arc_len += $mxy;
    }
    $lin = $mov_len ? $arc_len/$mov_len : 0;
    $vel = $moves ? $arc_len/$moves : 0;
    printf ("len=%3d moves=%d movg=%d,%d m2xy=%d m2sxy=%d dot=%3.1f al=%d,%d lin=%3.1f vel=%3.1f", 
	    $len, $moves, $lmoving, $moving, $mxy, $msxy, $dot, $arc_len, $mov_len, $lin, $vel);
}

sub evaluate_end ()
{
    emit("bang") if ($gesture_state eq IN_BANG);
    emit("touch") if ($gesture_state eq IN_TOUCH);
    emit("hold") if ($gesture_state eq IN_HOLD);
    print "done!";
}

sub emit ($)
{
    printf ("EMIT(%s) ", $_[0]);
}
