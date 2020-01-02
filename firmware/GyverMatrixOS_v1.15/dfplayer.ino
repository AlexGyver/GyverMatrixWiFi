void InitializeDfPlayer1() {
  mp3Serial.begin(9600);  
  dfPlayer.begin(mp3Serial, true, true);
  dfPlayer.setTimeOut(2000);
  dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  dfPlayer.volume(1);
}

void InitializeDfPlayer2() {    
  Serial.print(F("Инициализация MP3 плеера."));
  refreshDfPlayerFiles();    
  Serial.println(String(F("Звуков будильника найдено: ")) + String(alarmSoundsCount));
  Serial.println(String(F("Звуков рассвета найдено: ")) + String(dawnSoundsCount));
  isDfPlayerOk = alarmSoundsCount + dawnSoundsCount > 0;
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      //Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      //Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println(F("USB Inserted!"));
      break;
    case DFPlayerUSBRemoved:
      Serial.println(F("USB Removed!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number: "));
      Serial.print(value);
      Serial.println(F(". Play Finished!"));
      if (!(isAlarming || isPlayAlarmSound) && soundFolder == 0 && soundFile == 0) {
        dfPlayer.stop();
      }
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void refreshDfPlayerFiles() {
  // Чтение почему-то не всегда работает, иногда возвращает 0 или число от какого-то предыдущего запроса
  // Для того, чтобы наверняка считать значение - первое прочитанное игнорируем, потом читаем несколько раз до повторения.

  // Папка с файлами для будильника
  int cnt = 0, val = 0, new_val = 0; 
  do {
    val = dfPlayer.readFileCountsInFolder(1);     delay(10);
    new_val = dfPlayer.readFileCountsInFolder(1); delay(10);    
    if (val == new_val && val != 0) break;
    cnt++;
    delay(100);
    Serial.print(F("."));
  } while ((val = 0 || new_val == 0 || val != new_val) && cnt < 5);
  alarmSoundsCount = val;
  
  // Папка с файлами для рассвета
  cnt = 0, val = 0, new_val = 0; 
  do {
    val = dfPlayer.readFileCountsInFolder(2);     delay(10);
    new_val = dfPlayer.readFileCountsInFolder(2); delay(10);     
    if (val == new_val && val != 0) break;
    cnt++;
    delay(100);
    Serial.print(F("."));
  } while ((val = 0 || new_val == 0 || val != new_val) && cnt < 5);    
  dawnSoundsCount = val;
  Serial.println();  
}

void PlayAlarmSound() {
  int8_t sound = alarmSound;
  // Звук будильника - случайный?
  if (sound == 0) {
    sound = random(1, alarmSoundsCount);     // -1 - нет звука; 0 - случайный; 1..alarmSoundsCount - звук
  }
  // Установлен корректный звук?
  if (sound > 0) {
    dfPlayer.stop();
    delay(100);                              // Без этих задержек между вызовами функция dfPlayer приложение крашится.
    dfPlayer.volume(constrain(maxAlarmVolume,1,30));
    delay(100);
    dfPlayer.playFolder(1, sound);
    delay(100);
    dfPlayer.enableLoop();
    delay(100);    
    alarmSoundTimer.setInterval(alarmDuration * 60L * 1000L);
    alarmSoundTimer.reset();
    isPlayAlarmSound = true;
  } else {
    // Звука будильника нет - плавно выключить звук рассвета
    StopSound(1500);
  }
}

void PlayDawnSound() {
  // Звук рассвета отключен?
  int8_t sound = dawnSound;
  // Звук рассвета - случайный?
  if (sound == 0) {
    sound = random(1, dawnSoundsCount);     // -1 - нет звукаж 0 - случайный; 1..alarmSoundsCount - звук
  }
  // Установлен корректный звук?
  if (sound > 0) {
    dfPlayer.stop();
    delay(100);                             // Без этих задержек между вызовами функция dfPlayer приложение крашится.
    dfPlayer.volume(1);
    delay(100);
    dfPlayer.playFolder(2, sound);
    delay(100);
    dfPlayer.enableLoop();
    delay(100);
    // Установить время приращения громкости звука - от 1 до maxAlarmVolume за время продолжительности рассвета realDawnDuration
    fadeSoundDirection = 1;   
    fadeSoundStepCounter = maxAlarmVolume;
    if (fadeSoundStepCounter <= 0) fadeSoundStepCounter = 1;
    fadeSoundTimer.setInterval(realDawnDuration * 60L * 1000L / fadeSoundStepCounter);
    alarmSoundTimer.setInterval(4294967295);
  } else {
    StopSound(1500);
  }
}

void StopSound(int duration) {
  
  isPlayAlarmSound = false;

  if (duration <= 0) {
    dfPlayer.stop();
    delay(100);
    dfPlayer.volume(0);
    return;
  }
  
  // Чтение текущего уровня звука часто глючит и возвращает 0. Тогда использовать maxAlarmVolume
  fadeSoundStepCounter = dfPlayer.readVolume();
  if (fadeSoundStepCounter <= 0) fadeSoundStepCounter = maxAlarmVolume;
  if (fadeSoundStepCounter <= 0) fadeSoundStepCounter = 1;
    
  fadeSoundDirection = -1;   
  fadeSoundTimer.setInterval(duration / fadeSoundStepCounter);
}
