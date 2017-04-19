/*
 Copyright (C) 2016 Jonny Morrill jonny@morrill.me>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * @file RadConnect.h
 *
 * Class declaration for RadConnect and helper enums
 */

#ifndef __RAD_CONNECT_H__
#define __RAD_CONNECT_H__


// Device Types
enum DeviceType {
    SwitchBinary     = 1,
    SensorBinary     = 2,
    SwitchMultiLevel = 3,
    SensorMultiLevel = 4
};

// Message Types
enum MessageType {
    Command = 1,
    Event   = 2
};

// Command Types
enum CommandType {
    Get = 1,
    Set = 2
};

// Event Types
enum EventType {
    State = 1
};


typedef void (* SET_FP)(uint8_t);
typedef uint8_t (* GET_FP)(void);


class RadThing {

  private:

    DeviceType _type;
    char *name;

  public:

    RadThing(DeviceType type, char *name) { _type = type; _name = name };

    DeviceType getType() { return _type; };
    char *getName() { return _name; };

    virtual void callback(CommandType, SET_FP);
    virtual void callback(CommandType, GET_FP);
    virtual void command(CommandType, uint8_t, uint8_t*);
    virtual void event(EventType, uint8_t, uint8_t*);
};


class RadConnect
{

  private:

    // void discover(void);
    // void receiveMessages(void);

    // DeviceInfo *_devices[MAX_DEVICES];
    bool _started;
    vector<RadThing> _things;

  public:

    void add(DeviceType type, char *name);

    bool begin(void);
    void update(void);

};


class SwitchBinaryThing : public RadThing {

  private:

    SET_FP _set_callback;
    GET_FP _get_callback;

  public:

    SwitchBinaryThing()
    : RadThing(DeviceType type, char *name)
    {
      // pass? noop?
    }

    void callback(CommandType command_type, SET_FP func);
    void callback(CommandType command_type, GET_FP func);

    void command(CommandType command_type, uint8_t payload_length, uint8_t *payload);
    void event(EventType event_type, uint8_t payload_length, uint8_t *payload);

};


#endif // __RAD_CONNECT_H__
