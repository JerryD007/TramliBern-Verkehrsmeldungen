"""Decode standard GTFS-RT without silently discarding unknown constraints."""
from google.transit import gtfs_realtime_pb2 as pb
from google.protobuf.json_format import MessageToDict


def has_unknown(message):
    clean=type(message)()
    clean.CopyFrom(message)
    clean.DiscardUnknownFields()
    return clean.SerializeToString(deterministic=True)!=message.SerializeToString(deterministic=True)


def own_unknown(message, exclude=()):
    copy=type(message)()
    copy.CopyFrom(message)
    for name in exclude: copy.ClearField(name)
    return has_unknown(copy)


def protobuf_payload(data):
    feed=pb.FeedMessage()
    feed.ParseFromString(data)
    if not feed.IsInitialized(): raise ValueError('incomplete protobuf feed')
    if feed.header.gtfs_realtime_version!='2.0': raise ValueError('unsupported GTFS-RT version')
    if own_unknown(feed,('entity',)):
        raise ValueError('unknown protobuf feed/header constraints')
    result=MessageToDict(feed)
    result.setdefault('entity',[])
    for entity, row in zip(feed.entity,result['entity']):
        if not entity.HasField('alert'): continue
        alert=row['alert']
        if own_unknown(entity,('alert',)) or own_unknown(entity.alert,('informed_entity',)):
            alert['informedEntity']=[{'unsupportedProtobufFields':True}]
            continue
        selectors=alert.get('informedEntity',[])
        for original, selector in zip(entity.alert.informed_entity,selectors):
            if has_unknown(original):
                # Includes nested TripDescriptor extensions and unknown enum values.
                # Never promote remaining route/stop fields after losing a condition.
                selector['unsupportedProtobufFields']=True
    return result
