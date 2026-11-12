package com.example.homesec.exception;

public class HomesecException extends RuntimeException {

    public HomesecException() {
        super();
    }

    public HomesecException(String message) {
        super(message);
    }

    public HomesecException(Throwable cause) {
        super(cause);
    }

    public String getResponseMessage() {
        return "{\"Error\":\"" + super.getMessage() + "\"}";
    }

}
