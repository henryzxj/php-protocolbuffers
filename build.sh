#!/bin/sh
/usr/local/php7/bin/phpize && ./configure --with-php-config=/usr/local/php7/bin/php-config --enable-debug && make&& sudo make install
