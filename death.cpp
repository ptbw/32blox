/*
 * death.cpp - part of 32Blox, a breakout game for the 32blit built to
 * explore the API.
 *
 * If the player has reached a new high score, this is where we will ask them
 * to enter their initials so that their triumph is preserved for eternity.
 * Or at least until they turn their blit off.
 *
 * Please note that this is a first attempt at understanding a somewhat fluid
 * API on a shiny new bit of kit, so it probably is not full of 'best practice'.
 * It will hopefully serve as some sort of starting point, however.
 *
 * Coyright (C) 2020 Pete Favelle <pete@fsquared.co.uk>
 *
 * This software is provided under the MIT License. See LICENSE.txt for details.
 */

/* System headers. */

#include <string.h>


/* Local headers. */

#include "32blit.hpp"
#include "32blox.hpp"

//#include "32bee.h"


/* Module variables. */

static blit::Pen    m_text_colour;
static uint16_t     m_gradient_row;
static uint32_t     m_score;
static char         m_player[3];
static uint8_t      m_cursor;
static uint32_t     m_waiting;
static blit::Timer  m_wait_timer, m_flicker_timer;


/* Module functions. */

/*
 * _death_wait_timer_update - a callback simply to set the waiting flag to false
 */

void _death_wait_timer_update( blit::Timer &p_timer )
{
    m_waiting = false;
    p_timer.stop();
}


/*
 * _death_flicker_timer_update - callback for the font flicker and background
 */

void _death_flicker_timer_update( blit::Timer &p_timer )
{
  static uint16_t ls_loopcount = 0;
  
  /* Update the text colour used for flickeringness. */
  if ( ( ls_loopcount += 25 ) > 1200 ) 
  {
    ls_loopcount = 0;
  }
  m_text_colour = blit::Pen( 
                          ls_loopcount % 255, 
                          ( ls_loopcount % 512 ) / 2, 
                          255 - ( ls_loopcount % 255 ),
                          255
                        );
  m_gradient_row = ( ls_loopcount / 10 ) % 120;
}


/* Functions. */


/*
 * death_check_score - saves the new score, so that we know what it is later.
 * 
 * Returns a boolean flag, true if it qualifies for the hiscore table or false
 * if the player has fallen short.
 */

bool death_check_score( uint32_t p_score )
{
  /* Check this against the hiscore. */
  if ( hiscore_get_score( MAX_SCORES-1 ) < p_score )
  {
    m_wait_timer.init( _death_wait_timer_update, 250, 0 );
    m_score = p_score;
    m_player[0] = m_player[1] = m_player[2] = 'A';
    m_cursor = 0;
    return true;
  }
  
  /* Nothing doing, then. */
  return false;
}


/*
 * death_update - process the used inputting their name.
 *
 * Returns gamestate_t, the state to continue in. Should either be DEATH, 
 * or HISCORE when the user is ready to play.
 */

gamestate_t death_update( void )
{
  bool l_moving = false;
  
  /* If it's not running, we need to set up the flicker timer. */
  if ( !m_flicker_timer.started )
  {
    m_flicker_timer.init( _death_flicker_timer_update, 20, -1 );
    m_flicker_timer.start();
  }
  
  /* Move the cursor left. */
  if ( ( blit::pressed( blit::Button::DPAD_LEFT ) ) || ( blit::joystick.x < -0.1f ) )
  {
    /* Remember that we're moving somewhere. */
    l_moving = true;
    
    /* But only act on it if we're not waiting for a timeout. */
    if ( !m_waiting )
    {
      /* Keep the cursor in bounds, obviously. */
      if ( m_cursor > 0 )
      {
        m_waiting = true;
        m_wait_timer.start();
        m_cursor--;
      }
    }
  }
  
  /* Or right, come to that! */
  if ( ( blit::pressed( blit::Button::DPAD_RIGHT ) ) || ( blit::joystick.x > 0.1f ) )
  {
    /* Remember that we're moving somewhere. */
    l_moving = true;
    
    /* But only act on it if we're not waiting for a timeout. */
    if ( !m_waiting )
    {
      /* Keep the cursor in bounds, obviously. */
      if ( m_cursor < 2 )
      {
        m_waiting = true;
        m_wait_timer.start();
        m_cursor++;
      }
    }
  }
  
  /* Up means moving up through the alphabet. */
  if ( ( blit::pressed( blit::Button::DPAD_UP ) ) || ( blit::joystick.y < -0.1f ) )
  {
    /* Remember that we're moving somewhere. */
    l_moving = true;
    
    /* But only act on it if we're not waiting for a timeout. */
    if ( !m_waiting )
    {
      /* Increment the appropriate letter, within bounds. */
      if ( m_player[m_cursor] < 'Z' )
      {
        m_waiting = true;
        m_wait_timer.start();
        m_player[m_cursor]++;
      }
    }
  }
  
  /* And down means, well, moving down through the alphabet. */
  if ( ( blit::pressed( blit::Button::DPAD_DOWN ) ) || ( blit::joystick.y > 0.1f ) )
  {
    /* Remember that we're moving somewhere. */
    l_moving = true;
    
    /* But only act on it if we're not waiting for a timeout. */
    if ( !m_waiting )
    {
      /* Increment the appropriate letter, within bounds. */
      if ( m_player[m_cursor] > ' ' )
      {
        m_waiting = true;
        m_wait_timer.start();
        m_player[m_cursor]--;
      }
    }
  }
   
  /* If there's no user movement, reset the input timer. */
  if ( !l_moving )
  {
    m_waiting = false;
  }
  
  /* Check to see if the player has pressed the save button. */
  if ( blit::pressed( blit::Button::B ) )
  {
    /* Save this, and take the user into the hi score table. */
    hiscore_save_score( m_score, m_player );
    m_flicker_timer.stop();
    return STATE_HISCORE;
  }
  
  /* Default to the status quo, then. */
  return STATE_DEATH;
}


/* 
 * death_render - draw the player as she enters her name.
 */

void death_render( void )
{
  uint16_t    l_row;
  //bee_point_t l_point;
  //bee_font_t  l_outline_font, l_minimal_font;
  blit::Point l_point;
  
  double w = 64.0;
  double h = 48.0;
  
  /* Clear the screen to a nice shifting gradient. */
  for( l_row = 0; l_row < blit::screen.bounds.h; l_row++ )
  {        
    double s = (3.14159 * 2 / blit::screen.bounds.h) * l_row;
    double c = (3.14159 * 2 / blit::screen.bounds.h) * l_row;
    blit::screen.pen = 
      blit::Pen( 
        (int)( w + ( h *  sin( s ) ) ), 
        0, 
        (int)( w + ( h *  cos( c ) ) ), 
        255 
      );

   
    blit::screen.line( blit::Point( 0, ( l_row + m_gradient_row ) % blit::screen.bounds.h ), 
             blit::Point( blit::screen.bounds.w, ( l_row + m_gradient_row ) % blit::screen.bounds.h ) );
  }
  
  /* Frame everything with bricks; we're a brick game after all! */
  sprite_render( "brick_yellow", 0, 0 );
  sprite_render( "brick_yellow", 16, 0 );
  sprite_render( "brick_yellow", 0, 8 );

  sprite_render( "brick_yellow", 128, 0 );
  sprite_render( "brick_yellow", 144, 0 );
  sprite_render( "brick_yellow", 144, 8 );
  
  sprite_render( "brick_yellow", 0, 112 );
  sprite_render( "brick_yellow", 16, 112 );
  sprite_render( "brick_yellow", 0, 104 );

  sprite_render( "brick_yellow", 128, 112 );
  sprite_render( "brick_yellow", 144, 112 );
  sprite_render( "brick_yellow", 144, 104 );
  
  /* Get hold of the fonts in our new renderer. */
  //memcpy( &l_outline_font, bee_text_create_fixed_font( outline_font ), sizeof( bee_font_t ) );
  //memcpy( &l_minimal_font, bee_text_create_fixed_font( minimal_font ), sizeof( bee_font_t ) );
  
  /* Put the headings in somewhere sensible. */
  blit::screen.pen = blit::Pen( 255, 255, 255, 255 );
  //bee_text_set_font( &l_outline_font );
  l_point.x = (blit::screen.bounds.w / 2) - 40;
  l_point.y = 1;  
  //bee_text( &l_point, BEE_ALIGN_CENTRE, "NEW HIGH SCORE!" );
  blit::screen.text("NEW HIGH SCORE!", blit::outline_font, l_point); 
  l_point.x = (blit::screen.bounds.w / 2) - 20;
  l_point.y = 20;
  //bee_text( &l_point, BEE_ALIGN_CENTRE, "%05d", m_score );
  char line[20];
  sprintf(line, "%05d", m_score);
  blit::screen.text(line, blit::outline_font, l_point); 
  l_point.x = (blit::screen.bounds.w / 2) - 60;
  l_point.y = 64;
  //bee_text( &l_point, BEE_ALIGN_CENTRE, "LEFT/RIGHT TO SELECT" );
  blit::screen.text("LEFT/RIGHT TO SELECT", blit::outline_font, l_point); 
  l_point.x = (blit::screen.bounds.w / 2) - 50;
  l_point.y = 80;
  //bee_text( &l_point, BEE_ALIGN_CENTRE, "UP/DOWN TO CHANGE" );
  blit::screen.text("UP/DOWN TO CHANGE", blit::outline_font, l_point); 
  
  /* Now show the initials, in a different font to be distinctive. */
  //bee_text_set_font( &l_minimal_font );
  l_point.y = 40;
  l_point.x = ( blit::screen.bounds.w / 2 ) - 12;
  //bee_text( &l_point, BEE_ALIGN_CENTRE, "%c", m_player[0] );  
  sprintf(line, "%c", m_player[0]);
  blit::screen.text(line, blit::minimal_font, l_point); 
  l_point.x = ( blit::screen.bounds.w / 2 ) - 2;
  //bee_text( &l_point, BEE_ALIGN_CENTRE, "%c", m_player[1] );
  sprintf(line, "%c", m_player[1]);
  blit::screen.text(line, blit::minimal_font, l_point); 
  l_point.x = ( blit::screen.bounds.w / 2 ) + 8;
  //bee_text( &l_point, BEE_ALIGN_CENTRE, "%c", m_player[2] );
  sprintf(line, "%c", m_player[2]);
  blit::screen.text(line, blit::minimal_font, l_point); 
  
  /* Draw a cursor around the currently selected letter. */
  blit::screen.pen = m_text_colour;
  blit::screen.line( blit::Point( ( blit::screen.bounds.w / 2 ) - 14 + ( 10 * m_cursor ), 38 ), 
                 blit::Point( ( blit::screen.bounds.w / 2 ) - 6 + ( 10 * m_cursor ), 38 ) );
  blit::screen.line( blit::Point( ( blit::screen.bounds.w / 2 ) - 6 + ( 10 * m_cursor ), 38 ), 
                 blit::Point( ( blit::screen.bounds.w / 2 ) - 6 + ( 10 * m_cursor ), 48 ) );
  blit::screen.line( blit::Point( ( blit::screen.bounds.w / 2 ) - 6 + ( 10 * m_cursor ), 48 ), 
                 blit::Point( ( blit::screen.bounds.w / 2 ) - 14 + ( 10 * m_cursor ), 48 ) );
  blit::screen.line( blit::Point( ( blit::screen.bounds.w / 2 ) - 14 + ( 10 * m_cursor ), 48 ), 
                 blit::Point( ( blit::screen.bounds.w / 2 ) - 14 + ( 10 * m_cursor ), 38 ) );
  
  /* Lastly, the text inviting the user to press the start button. */
  blit::screen.pen = m_text_colour;
  //bee_text_set_font( &l_outline_font );
  l_point.x = (blit::screen.bounds.w / 2) - 50;
  l_point.y = 100;
  //bee_text( &l_point, BEE_ALIGN_CENTRE, "PRESS 'B' TO SAVE" );
  blit::screen.text("PRESS 'B' TO SAVE", blit::outline_font, l_point); 
}


/* End of death.cpp */
