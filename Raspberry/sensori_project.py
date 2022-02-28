import board
import busio
import RPi.GPIO as GPIO
import serial
import math
import re
import time
import datetime
import numpy as np
import Adafruit_CharLCD as LCD

lcd_rs = 26
lcd_en = 11
lcd_d4 = 19
lcd_d5 = 13
lcd_d6 = 6
lcd_d7 = 5
lcd_backlight = 4

lcd_columns = 16
lcd_rows = 2

lcd = LCD.Adafruit_CharLCD(lcd_rs, lcd_en, lcd_d4, lcd_d5, lcd_d6, lcd_d7, lcd_columns, lcd_rows, lcd_backlight)
lcd.clear()

GPIO.setmode(GPIO.BCM)
GPIO.setup(23,GPIO.OUT)
GPIO.output(23,GPIO.LOW)

def main():
    REPETITION = 5
    
    
    v_ref = 3.3
    v_min = 0
    sensor_accuracy = 0.2
    interface_accuracy = 1
    n_bit = 12
    
    response = input("Hai utilizzato sensori che introducono un guadagno moltiplicativo nelle misure effettuate? (S/N): ").upper()
    if response == "S":
        v_gain = int(input("Guadagno per la tensione: "))
        i_gain = int(input("Guadagno per la corrente: "))
    else:
        v_gain = 1
        i_gain = 1
        
    division = input("Il tuo segnale originario assume anche valori negativi? (S/N): ").upper()
    
    
    while True:
        
        response = input("Inserisci l'intervallo di tempo per cui vuoi stimare il consumo energetico (h): ")
        TIME = int(response)
        
        p_m_array = []
        p_app_array = []
        q_array = []
        phase_array = []
        v_max_array = []
        i_max_array = []
        
        for i in range(REPETITION):
            
            serialport = serial.Serial("/dev/ttyS0", baudrate=9600)
            
            measures = []
            serialport.reset_output_buffer()
            readMeasures(measures, serialport)
            serialport.close()

            v_array = measures[1].split(" ")
            v_array = list(filter(None, v_array))
            v_measure = np.array(v_array[0:len(v_array)-1]).astype(np.float)
            v_measure = ((v_measure*3.3)/4095)/v_gain
            if division == "S":
                v_measure = v_measure - np.amax(v_measure)/2
            v_max = np.amax(v_measure)
            v_max_array.append(v_max)
            
            i_array = measures[3].split(" ")
            i_array = list(filter(None, i_array))
            i_measure = np.array(i_array[0:len(i_array)-1]).astype(np.float)
            i_measure = ((i_measure*3.3)/4095)/i_gain
            if division == "S":
                i_measure = i_measure - np.amax(i_measure)/2
            i_max = np.amax(i_measure)
            i_max_array.append(i_max)
            
            phase = phase_from_string(measures[5])
            phase = check_phase(phase)
            
            phase_array.append(phase)
            
            p_app_array.append((v_max*i_max/2))
            p_m_array.append(v_max*i_max/2 * np.cos(min(phase, 90)*np.pi/180))
            q_array.append(v_max*i_max/2 * np.sin(min(phase, 90)*np.pi/180))
            
        
        separator = "------------------------------------------------------------------------------"
        print("\n", separator)
            
        u_phase = uncertaintyPhaseShift(phase_array)
        phase_avg = np.mean(np.array(phase_array))
        print("Angolo di fase medio: ", phase_avg, "+-", u_phase, "\n", separator)
        
        
        v_max = np.mean(np.array(v_max_array))
        i_max = np.mean(np.array(i_max_array))
        
        uv = uncertaintyV(v_max, v_ref, v_min, n_bit, sensor_accuracy, interface_accuracy)
        ui = uncertaintyI(i_max, v_ref, v_min, n_bit, sensor_accuracy, interface_accuracy)
        print("Incertezza della tensione: ", uv)
        print("Incertezza della corrente: ", ui)
        print(separator)
        
        u_cos_phase = uncertaintyCosPhaseShift(phase_array)
        u_sin_phase = uncertaintySenPhaseShift(phase_array)
        
        u_apparent_power = uncertaintyPropagatedApparentPower(ui, uv)
        u_active_power = uncertaintyPropagatedActivePower(ui,uv,u_cos_phase)
        u_reactive_power = uncertaintyPropagatedReactivePower(ui, uv, u_sin_phase)
        
        p_avg = np.mean(np.array(p_m_array))*TIME
        p_app = np.mean(np.array(p_app_array))*TIME
        q = np.mean(np.array(q_array))*TIME
        
        print("Misure di potenza:")
        print("Potenza attiva consumata in", TIME, "h : ", p_avg,"+- ", u_active_power, "Wh")
        print("Potenza apparente consumata in", TIME, "h : ", p_app, "+- ", u_apparent_power, "Wh")
        print("Potenza reattiva consumata in", TIME, "h : ", q, "+- ", u_reactive_power, "VARh")
        print("Fattore di potenza in", TIME, "h : ", p_avg/p_app)
        print(separator)
        
        lcd.clear()
        lcd.message("P_avg: " + str(round(p_avg, 3)) + " Wh")
        lcd.message("\nP_app: " + str(round(p_app, 3)) + " Wh")
        time.sleep(3)
        lcd.clear()
        lcd.message("Q: " + str(round(q, 3)) + " VARh")
        lcd.message("\nP_factor: " + str(round(p_avg/p_app, 3)))
        
        f = open("storico.txt", "a")
        msg = "Consumo energetico " + str(datetime.datetime.now()) + " per " + str(TIME) + " ore di utilizzo:\n" + "   - P_avg: " + str(p_avg) + " Wh \n" + "   - P_app: " + str(p_app) + " Wh \n" + "   - Q: " + str(q) + " VARh \n\n"
        f.write(msg)
        f.close()
        
def uncertaintyV(V, Vref, Vmin, n_bit, sensor_accuracy = 0.2, interface_accuracy = 1):
    u_adc = ((Vref/(2**n_bit))*0.5)
    u_sens = V * sensor_accuracy/100
    u_interface = V*interface_accuracy/100
    return math.sqrt(u_adc**2 + u_sens**2 + u_interface**2) 
    
def uncertaintyI(I, Vref, Vmin, n_bit, sensor_accuracy = 0.2, interface_accuracy = 1):
    u_adc = ((Vref/2**n_bit)*0.5)
    u_sens = I * sensor_accuracy/100
    u_interface = I*interface_accuracy/100
    return math.sqrt(u_adc**2 + u_sens**2 + u_interface**2)

def uncertaintyPhaseShift(l):
    phase_array = np.array(l, dtype = "float")
    variance = np.var(phase_array)
    u = np.sqrt(variance/len(phase_array))
    return u

def uncertaintyCosPhaseShift(l):
    phase_cos_array = np.cos(np.array(l, dtype = "float"))    
    variance = np.var(phase_cos_array)
    u = np.sqrt(variance/len(phase_cos_array))
    return u

def uncertaintySenPhaseShift(l):
    phase_sin_array = np.sin(np.array(l, dtype = "float"))
    variance = np.var(phase_sin_array)
    u = np.sqrt(variance/len(phase_sin_array))
    return u

def uncertaintyPropagatedActivePower(ui, uv, ucosPhase):
    return math.sqrt(ui**2 + uv**2 + ucosPhase**2)

def uncertaintyPropagatedReactivePower(ui, uv, usenPhase):
    return math.sqrt(ui**2 + uv**2 + usenPhase**2)

def uncertaintyPropagatedApparentPower(ui, uv):
    return math.sqrt(ui**2 + uv**2)

    
def readMeasures(measures, serialport):
    GPIO.output(23,GPIO.HIGH)
    for i in range(6):
        measures.append(serialport.readline().decode('UTF-8').replace("\x00", ""))
    GPIO.output(23,GPIO.LOW)

def phase_from_string(str1): 
    start = 0
    if str1[0] == "-":
        first = int(str1[0:2])
        start = 1
    else:
        first = int(str1[0])
        start = 0
    i = 1

    int_part = 0
    for i in range(start, len(str1), 1):
        if not str1[i].isdigit():
            int_part = i
            break
    
    last = int_part
    for i in range(int_part + 1, len(str1), 1):
        if not str1[i].isdigit():
            last = i
            break
        
    return float(str1[0: last])

def check_phase(phase):
    
    if(phase < -90 or phase > 90):
        if(phase < -90):
            return (phase + 360) if (abs((phase+360)-90)< abs(phase + 90)) else phase
        else:
            return (phase - 360) if (abs((phase-360)+90)< abs(phase - 90)) else phase
    return phase
   
        
if __name__ == "__main__":
    main()

