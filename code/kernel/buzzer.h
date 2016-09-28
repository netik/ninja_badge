#ifndef BUZZER_H
#define BUZZER_H

void buzzer_load(unsigned short comp1, unsigned short comp2);
void buzzer_reload(unsigned short comp1, unsigned short comp2);
void buzzer_start(void);
void buzzer_init(void);
void buzzer_stop(void);

void play_attacked_tone(void);
void play_dodge_tone(void);
void play_hit_tone(void);
void play_defeat_tone(void);
void play_victory_tone(void);

#endif /* BUZZER_H */
