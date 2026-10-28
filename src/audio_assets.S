.section .rodata
.balign 16
.global asset_audio_irx_start
.global asset_audio_irx_end
asset_audio_irx_start:
.incbin "assets/converted/audsrv.irx"
asset_audio_irx_end:
.balign 16
.global asset_music_start
.global asset_music_end
asset_music_start:
.incbin "assets/converted/music.adp"
asset_music_end:
.balign 16
.global asset_eat_start
.global asset_eat_end
asset_eat_start:
.incbin "assets/converted/eat.adp"
asset_eat_end:
.balign 16
.global asset_dash_start
.global asset_dash_end
asset_dash_start:
.incbin "assets/converted/dash.adp"
asset_dash_end:
.balign 16
.global asset_grow_start
.global asset_grow_end
asset_grow_start:
.incbin "assets/converted/grow.adp"
asset_grow_end:
.balign 16
.global asset_frenzy_start
.global asset_frenzy_end
asset_frenzy_start:
.incbin "assets/converted/frenzy.adp"
asset_frenzy_end:
.balign 16
.global asset_death_start
.global asset_death_end
asset_death_start:
.incbin "assets/converted/death.adp"
asset_death_end:
.balign 16
.global asset_game_over_start
.global asset_game_over_end
asset_game_over_start:
.incbin "assets/converted/game_over.adp"
asset_game_over_end:
.balign 16
.global asset_extra_life_start
.global asset_extra_life_end
asset_extra_life_start:
.incbin "assets/converted/extra_life.adp"
asset_extra_life_end:
.balign 16
