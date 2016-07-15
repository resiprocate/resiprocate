## Releasing

* Make sure the `ChangeLog` is up to date.
* Edit `configure.ac` and bump the version
* Run the following:

        ./bootstrap
        ./configure
        make check -j 4
        sudo make install

* `make dist`
* Check that you can untar this release and install it
* `git tag v{X.Y.Z}`
* `git push --tags`
* Make a new release on GitHub at https://github.com/maxmind/geoip-api-c/releases
    * Upload the tarball you just made
    * Edit said release to include the changes for this release on GitHub


## Ubuntu PPA packages

We can probably script most of this but this is the current process:

0. Switch to the `ubuntu-ppa` branch and merge the release tag from above.
1. Type `dch -i` and add the appropriate `debian/changelog` entry.
2. Move tarball created above to a temp directory and
   name it `geoip_1.?.?.orig.tar.gz`.
3. Unpack tarball.
4. Copy `debian` directory from Git. (We intentionally do not include it in
   the tarball so that we don't interfere with Debian's packaging.)
5. Update `debian/changelog` for the dist you are releasing to, e.g.,
   precise, trusty, vivid, and prefix the version with the a `~` followed
   by the dist name, e.g., `1.6.3-1+maxmind1~trusty`.
6. Run `debuild -S -sa -rfakeroot -k<KEY>`. (The key may not be necessary
   if your .bashrc is appropriately )
7. Run `lintian` to make sure everything looks sane.
8. Run `dput ppa:maxmind/ppa ../<source.changes files created above>` to
   upload.
9. Repeat 4-8 for remaining distributions.

## Homebrew

* Update the [Homebrew formula](https://github.com/Homebrew/homebrew/blob/master/Library/Formula/geoip.rb).
