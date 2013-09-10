#!/bin/bash

YEAR=2012
AUTHOR="Daniel Pocock"

TARGET_FILES=`find . -type f`

sed -i '/The Vovida Software License/ d' $TARGET_FILES

sed -i "s/Copyright (c) 2000 Vovida Networks, Inc/Copyright $YEAR $AUTHOR/" $TARGET_FILES

sed -i '/The names "VOCAL", "Vovida Open Communication Application Library"/,+3 d' $TARGET_FILES
sed -i 's/   permission, please contact vocal@vovida.org./3. Neither the name of the author(s) nor the names of any contributors\
 *    may be used to endorse or promote products derived from this software\
 *    without specific prior written permission./' $TARGET_FILES

sed -i '/4. Products derived from this software may not be called "VOCAL"/,+3 d' $TARGET_FILES

sed -i 's/THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED/THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND\
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE\
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\
 * SUCH DAMAGE./' $TARGET_FILES

sed -i '/WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES/,+11 d' $TARGET_FILES

sed -i '/This software consists of voluntary contributions made by Vovida/,+3 d' $TARGET_FILES






