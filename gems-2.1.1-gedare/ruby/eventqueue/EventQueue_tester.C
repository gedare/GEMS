
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Ruby Multiprocessor Memory System Simulator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Ruby was originally developed primarily by Milo Martin and Daniel
    Sorin with contributions from Ross Dickson, Carl Mauer, and Manoj
    Plakal.

    Substantial further development of Multifacet GEMS at the
    University of Wisconsin was performed by Alaa Alameldeen, Brad
    Beckmann, Jayaram Bobba, Ross Dickson, Dan Gibson, Pacia Harper,
    Derek Hower, Milo Martin, Michael Marty, Carl Mauer, Michelle Moravan,
    Kevin Moore, Andrew Phelps, Manoj Plakal, Daniel Sorin, Haris Volos, 
    Min Xu, and Luke Yen.
    --------------------------------------------------------------------

    If your use of this software contributes to a published paper, we
    request that you (1) cite our summary paper that appears on our
    website (http://www.cs.wisc.edu/gems/) and (2) e-mail a citation
    for your published paper to gems@cs.wisc.edu.

    If you redistribute derivatives of this software, we request that
    you notify us and either (1) ask people to register with us at our
    website (http://www.cs.wisc.edu/gems/) or (2) collect registration
    information and periodically send it to us.

    --------------------------------------------------------------------

    Multifacet GEMS is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    Multifacet GEMS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Multifacet GEMS; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307, USA

    The GNU General Public License is contained in the file LICENSE.

### END HEADER ###
*/

/*
 * $Id$
 *
 */

#include "EventQueue.h"
#include "Consumer.h"

//static int global_counter = 0;

class TestConsumer1 : public Consumer {
public:
  TestConsumer1(int description) { m_description = description; }
  ~TestConsumer1() { }
  void wakeup() { cout << "Wakeup#1: " << m_description << endl; }
  // void wakeup() { global_counter++; }  
  void print(ostream& out) const { out << "1:" << m_description << endl; } 
  
private:
  int m_description;
};

class TestConsumer2 : public Consumer {
public:
  TestConsumer2(int description) { m_description = description; }
  ~TestConsumer2() { }
  void wakeup() { cout << "Wakeup#2: " << m_description << endl; }
  // void wakeup() { global_counter++; }  
  void print(ostream& out) const { out << "2:" << m_description << endl; }
private:
  int m_description;
};

int main()
{
  EventQueue q;
  const int SIZE = 200;
  const int MAX_TIME = 10000;
  int numbers[SIZE];
  Consumer* consumers[SIZE];

  for (int i=0; i<SIZE; i++) {
    numbers[i] = random() % MAX_TIME;
    if (i%2 == 0) {
      consumers[i] = new TestConsumer1(i);
    } else {
      consumers[i] = new TestConsumer2(i);
    }
  }

  for(int i=0; i<SIZE; i++) {
    q.scheduleEvent(consumers[i], numbers[i]);
  }

  q.triggerEvents(MAX_TIME);

  for (int i=0; i<SIZE; i++) {
    delete consumers[i];
  }
}
