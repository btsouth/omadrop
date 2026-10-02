#!/usr/bin/env python3
"""Targeted regressions for actual Zenity output and preference safety."""
import os
import runpy
import tempfile
from unittest.mock import patch
from types import SimpleNamespace

helper = runpy.run_path(os.path.join(os.path.dirname(__file__), '../bin/omadrop-effects'))
with tempfile.TemporaryDirectory() as config:
    with patch.dict(os.environ, {'XDG_CONFIG_HOME': config}):
        choose = helper['choose_checklist']
        with patch('subprocess.run', return_value=SimpleNamespace(returncode=0, stdout='beams|fireworks\n')):
            assert choose('Favorites', '', [('beams', 'Beams'), ('fireworks', 'Fireworks')], set()) == ['beams', 'fireworks']
        prefs = helper['Prefs'](['beams', 'fireworks'], ['burn'], ['# preserved', 'future-key=value'])
        helper['write_prefs'](prefs)
        loaded = helper['read_prefs']({'beams', 'fireworks', 'burn'})
        assert loaded.favorites == ['beams', 'fireworks'] and loaded.hidden == ['burn']
        path = helper['prefs_path']()
        assert os.stat(path).st_mode & 0o777 == 0o600
        before = open(path).read()
        assert '# preserved' in before and 'future-key=value' in before
        assert helper['noninteractive']([], prefs, {'beams'}, ['--favorite', 'beams', '--bogus']) == 2
        assert open(path).read() == before
        with patch('subprocess.run', return_value=SimpleNamespace(returncode=1, stdout='beams')):
            assert choose('Favorites', '', [], set()) is None
        assert open(path).read() == before
        valid = {'beams', 'fireworks', 'burn', 'swarm'}
        with open(path, 'w') as output:
            output.write('version=1\nfavorites=\nhidden=\n# original\n')
        browser = helper['read_prefs'](valid)
        external = helper['read_prefs'](valid)
        assert helper['noninteractive']([], external, valid, ['--favorite', 'swarm']) == 0
        external.extras.extend(['# added while browser was open', 'new-key=new-value'])
        helper['write_prefs'](external)
        helper['toggle_hidden'](browser, 'burn', len(valid))
        helper['write_prefs'](browser)
        merged = helper['read_prefs'](valid)
        assert merged.favorites == ['swarm'] and merged.hidden == ['burn']
        assert '# added while browser was open' in merged.extras
        assert 'new-key=new-value' in merged.extras
        assert browser.original_favorites == ['swarm']
        assert browser.original_hidden == ['burn']

        first = helper['read_prefs'](valid)
        second = helper['read_prefs'](valid)
        helper['toggle_favorite'](first, 'beams')
        helper['toggle_favorite'](second, 'fireworks')
        helper['write_prefs'](first)
        helper['write_prefs'](second)
        assert helper['read_prefs'](valid).favorites == ['swarm', 'beams', 'fireworks']
        # A later edit from the same browser must use its refreshed snapshot.
        helper['toggle_favorite'](second, 'beams')
        helper['write_prefs'](second)
        assert helper['read_prefs'](valid).favorites == ['swarm', 'fireworks']
        assert os.stat(path).st_mode & 0o777 == 0o600
        with open(path, 'w') as output:
            output.write('version=2\nfavorites=newer-format\n')
        try:
            helper['write_prefs'](prefs)
        except helper['PrefsError']:
            pass
        else:
            raise AssertionError('clobbered future version after a dialog opened')
        assert open(path).read() == 'version=2\nfavorites=newer-format\n'
print('preferences: Zenity separators, cancel, concurrent edits, unknown keys, atomic permissions, invalid CLI and future-version protection passed')
