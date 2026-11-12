package com.example.homesec.repository;

import com.example.homesec.persistence.AlarmEvent;
import org.springframework.data.repository.CrudRepository;
import org.springframework.stereotype.Repository;

import java.time.ZonedDateTime;
import java.util.List;

@Repository
public interface AlarmEventDAO extends CrudRepository<AlarmEvent, Long> {

    List<AlarmEvent> findByDeviceIdAndCreatedAfterOrderByCreatedDesc(String deviceId, ZonedDateTime after);
}
