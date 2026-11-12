package com.example.homesec.service;

import com.example.homesec.persistence.AlarmEvent;
import com.example.homesec.repository.AlarmEventDAO;
import jakarta.annotation.Resource;
import lombok.AllArgsConstructor;
import org.springframework.stereotype.Service;

import java.time.ZonedDateTime;
import java.util.List;

@Service
@AllArgsConstructor
public class AlarmEventService {

    @Resource
    private AlarmEventDAO alarmEventDAO;

    public AlarmEvent save(String deviceId, String source, boolean active, Float ppm, ZonedDateTime created) {
        AlarmEvent event = AlarmEvent.builder()
                .deviceId(deviceId)
                .source(source)
                .active(active)
                .coPpm(ppm)
                .created(created)
                .build();

        return alarmEventDAO.save(event);
    }

    public List<AlarmEvent> findSince(String deviceId, ZonedDateTime since) {
        return alarmEventDAO.findByDeviceIdAndCreatedAfterOrderByCreatedDesc(deviceId, since);
    }
}
